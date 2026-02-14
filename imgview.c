#define SDL_MAIN_USE_CALLBACKS
#include <SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <SDL_shadercross.h>
#include <SDL_image.h>

#include "ok_lib.h"
#define PLUTOSVG_BUILD_STATIC
#define PLUTOVG_BUILD_STATIC
#include <plutosvg/plutosvg.h>
#include <windows.h>


#define bufflen 1024
char buffer [ bufflen ];

SDL_Window *window;
SDL_Rect window_rect;
SDL_GPUDevice *device;
SDL_GPUGraphicsPipeline *pipeline;
SDL_GPUSampler *sampler[2];
SDL_GPUBuffer *index_buffer;

SDL_GPUComputePipeline* blur_pipeline;

typedef struct{
	int input_w, input_h;
    int output_w, output_h;
} blur_uniforms;

SDL_GPUGraphicsPipeline* wireframe_pipeline;

typedef struct{
	SDL_FPoint window;
	SDL_FPoint A;
	SDL_FPoint B;
	float  C;
	int mode;
} wireframe_uniforms;

// Flex Quad mode:
enum{ FM_crosshair = 0, FM_rect, FM_corners, FM_diamond };
const int FM_verts[] = { 4, 8, 16, 8 };

int width;
int height;
int W, H;//total width and height of the contents onscreen
float aspect_correction;

bool CTRL, SHIFT;
int mouseX, mouseY, pmouseX, pmouseY;

float cx;
float cy;
bool mousePressed;
int clickX, clickY;

int antialiasing = 0;
bool fit = false;
bool animating = false;
bool enable_blur = false;

SDL_FRect sel_rect;
int angle_i;
float ANGLE;
SDL_FlipMode FLIP;

bool mmpan;// middle mouse pan
bool zoom_in;
bool zoom_out;
bool pan_up;
bool pan_down;
bool pan_left;
bool pan_right;
bool rotate_ccw;
bool rotate_cw;

float zoomV;
float panV;
float panVF;//vel factor for the mmpan

bool fullscreen;
//bool minimized;
int update;

const SDL_FColor bg [] = { {0   , 0   , 0   , 1.0},
						   {0.25, 0.25, 0.25, 1.0},
						   {0.5 , 0.5 , 0.5 , 1.0},
						   {0.75, 0.75, 0.75, 1.0},
						   {1.0 , 1.0 , 1.0 , 1.0} };
int sel_bg = 1;



int SDL_framerateDelay( int frame_period ){
	static Uint64 then = 0;
	Uint64 now = SDL_GetTicks();
	int elapsed = now - then;
	int delay = frame_period - elapsed;
	//printf("%d - (%d - %d) = %d\n", frame_period, now, then, delay );
	if( delay > 0 ){
		SDL_Delay( delay );
		elapsed += delay;
	}
	then = SDL_GetTicks();
	return elapsed;
}


#define HALF_PI 1.5707963267948966192313216916397514420985846996875529104

// cycle indices "array style"
int cycle( int a, int min, int max ){
	if( a < min ) return max-1;
	else if( a >= max ) return min;
	else return a;
}

double map(double value, double source_lo, double source_hi,  double dest_lo, double dest_hi) {
	return dest_lo + (dest_hi - dest_lo) * ((value - source_lo) / (source_hi - source_lo));
}


typedef struct tvert_struct{
	float x, y, z;
	float u, v;
} tvert;

typedef struct Matrix4x4 {
	float m11, m12, m13, m14;
	float m21, m22, m23, m24;
	float m31, m32, m33, m34;
	float m41, m42, m43, m44;
} Matrix4x4;

typedef struct uni_struct{
	Matrix4x4 transform;
	float w, h;
} UniDat;

void Log_M4x4( Matrix4x4 *M ){
	SDL_Log( "[%g  %g  %g  %g]", M->m11, M->m12, M->m13, M->m14 );
	SDL_Log( "[%g  %g  %g  %g]", M->m21, M->m22, M->m23, M->m24 );
	SDL_Log( "[%g  %g  %g  %g]", M->m31, M->m32, M->m33, M->m34 );
	SDL_Log( "[%g  %g  %g  %g]", M->m41, M->m42, M->m43, M->m44 );
}

typedef struct transform_struct{
	float tx, ty; // translate
	float scale;
	float scale_i; // index, scale = 1.1 ^ scale_i
} Transform;

Transform T = (Transform){0,0,1,0};

double logarithm( double base, double x ){
	return SDL_log10( x ) / SDL_log10( base );
}

SDL_Rect apply_transform_rect( SDL_Rect *rct, Transform *T ){
	return (SDL_Rect){ SDL_lround( T->tx + (T->scale * rct->x) ), 
					   SDL_lround( T->ty + (T->scale * rct->y) ), 
					   SDL_lround( rct->w * T->scale ), 
					   SDL_lround( rct->h * T->scale ) };
}

Matrix4x4 transformToMatrix4x4( Transform* T, float rotation, int flip) {

	Matrix4x4 out = {0};

	float cos_r = SDL_cosf(rotation);
	float sin_r = SDL_sinf(rotation);

	float flip_x = (flip & SDL_FLIP_HORIZONTAL) ? -1.0f : 1.0f;
	float flip_y = (flip & SDL_FLIP_VERTICAL)   ? -1.0f : 1.0f;

	float icx = W * 0.5f;
	float icy = H * 0.5f;

	// M = S * T * c * R * F * -c
	out.m11 = T->scale * cos_r * flip_x;
	out.m12 = T->scale * sin_r * flip_y;
  //out.m13 = 0;
	out.m14 = (T->scale * cos_r * flip_x *-icx) + (T->scale * sin_r * flip_y * -icy) + (T->scale * icx) + T->tx;
	out.m21 = T->scale * -sin_r * flip_x;
	out.m22 = T->scale * cos_r * flip_y;
  //out.m23 = 0;
	out.m24 = (T->scale * -sin_r * flip_x * -icx) + (T->scale * cos_r * flip_y * -icy) + (T->scale * icy) + T->ty;
  //out.m31 = 0;
  //out.m32 = 0;
	out.m33 = 1;
  //out.m34 = 0;
  //out.m41 = 0;
  //out.m42 = 0;
  //out.m43 = 0;
	out.m44 = 1;

	return out;
}


void fit_rect( SDL_FRect *A, SDL_Rect *B ){
	float Ar = A->w / (float) A->h;
	float Br = B->w / (float) B->h;
	if( Ar > Br ){
		int h = A->h * (B->w / (float) A->w);
		A->x = B->x;
		A->y = B->y + ((B->h - h) / 2);
		A->w = B->w;
		A->h = h;
	}
	else {
		int w = A->w * (B->h / (float) A->h);
		A->x = B->x + ((B->w - w) / 2);
		A->y = B->y;
		A->w = w;
		A->h = B->h;
	}
}

SDL_Rect intersect_rects(const SDL_Rect* A, const SDL_Rect* B)
{
	SDL_Rect result = {0, 0, 0, 0};
	
	int   left = SDL_max(A->x, B->x);
	int  right = SDL_min(A->x + A->w, B->x + B->w);
	int    top = SDL_max(A->y, B->y);
	int bottom = SDL_min(A->y + A->h, B->y + B->h);

	if (left < right && top < bottom) {
		result.x = left;
		result.y = top;
		result.w = right - left;
		result.h = bottom - top;
	}
	
	return result;
}


//=================================== /Basic tooling
//=================================== Stringlist

typedef struct ok_vec_of(const char *) str_vec;

str_vec directory_list;


void destroy_str_vec( str_vec *v ){

	ok_vec_foreach_rev( v, char *str ) {
		SDL_free( str );
	}
	ok_vec_deinit( v );
}

//reverse cmp
int strrcmp( char *A, char *B ){
	size_t lA = SDL_strlen(A);
	size_t lB = SDL_strlen(B);
	int l = SDL_min( lA, lB );
	for (int i = 1; i <= l; ++i ){
		int r = A[lA-i] - B[lB-i];
		if( r != 0 ) return r;
	}
	return 0;
}

// sub-string
char* substr( char *string, int start, int stop ){
	char *sub = (char*) SDL_calloc( stop-start +1, sizeof(char) );
	//for (int i = start; i < stop; ++i){ 	sub[i-start] = string[i]; }
	SDL_memcpy( sub, string + start, stop-start );
	sub[ stop-start ] = '\0';
	return sub;
}

//=================================== /Stringlist
//=================================== File io



char *folderpath = NULL;
int folderpath_len = 0;
int INDEX = 0;// of the present file in the list
bool remote_operation = false;


void CP_ACP_to_UTF8(char *output, const char* input) {
	// from the system's code page (e.g., CP_ACP) to UTF-16 (wide characters)
	int wideCharLength = MultiByteToWideChar(CP_ACP, 0, input, -1, NULL, 0);

	wchar_t* wideCharStr = SDL_malloc(wideCharLength * sizeof(wchar_t));

	//to wide-char (UTF-16)
	MultiByteToWideChar(CP_ACP, 0, input, -1, wideCharStr, wideCharLength);

	//from UTF-16 to UTF-8
	int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, NULL, 0, NULL, NULL);

	WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, output, utf8Length, NULL, NULL);

	SDL_free(wideCharStr);
	//return utf8Str;
}

int check_extension( char *filename ){
	//                           1       2       3       4       5      6        7       8      9       10
	const char exts [][8] = { ".png", ".jpg", "jpeg", ".gif", ".tif", "tiff", ".ico", ".bmp", "webp", ".svg" };

	size_t len = SDL_strlen( filename );
	for (int i = 0; i < 10; ++i ){
		if( SDL_strcasecmp( filename + len -4, exts[i] ) == 0 ){
			return i+1;
		}
	}
	return 0;
}

SDL_EnumerationResult enudir_callback(void *userdata, const char *dirname, const char *fname){

	char *name = fname;
	size_t dl = SDL_strlen(dirname);
	if( dl > folderpath_len ){
		SDL_snprintf( buffer, bufflen, "%s%s", dirname + folderpath_len, fname );
		name = buffer;
	}
	size_t len = SDL_strlen( name );
	//SDL_Log(">buf:[%s]\n", name );

	if( check_extension( fname ) ){
		const char **neo = ok_vec_push_new( (str_vec*)userdata );
		*neo = SDL_calloc( len+1, sizeof(char) );
		SDL_memcpy( *neo, name, len+1 );
	}
	else{
		char *path = name;
		if( remote_operation ){
			SDL_snprintf( buffer+512, 512, "%s%s", dirname, fname );
			path = buffer+512;
		}
		SDL_PathInfo info = {0};
		SDL_GetPathInfo( path, &info );
		if( info.type == SDL_PATHTYPE_DIRECTORY ){
			//SDL_Log("  found dir: [%s]", name );
			const char **neo = ok_vec_push_new( (str_vec*)userdata );
			*neo = SDL_calloc( len+2, sizeof(char) );
			SDL_snprintf( *neo, len+2, "%s\\", name );
		}
	}
	return SDL_ENUM_CONTINUE;
}

void shuffle_str_list( const char **deck, int len ){
	for (int i = 0; i < len-2; ++i){
		int ni = i+1 + SDL_rand( len - (i+1) );
		char *temp = deck[i];
		deck[i] = deck[ni];
		deck[ni] = temp;
	}
}

void load_folderlist( str_vec *list, char *pfname, int depth ){

	//SDL_Log("load_folderlist( %s, %d );\n", pfname, depth );

	ok_vec_init( list );
	if( !SDL_EnumerateDirectory( folderpath, enudir_callback, list ) ){
		SDL_Log("SDL_EnumerateDirectory (1) error: %s", SDL_GetError());
		SDL_Log(">{%s}", folderpath );
	}
	depth -= 1;
	while( depth > 0 ){
		//SDL_Log("looking for subfolders...");
		int new_subdirs = 0;
		ok_vec_foreach_rev( list, char *p ) {
			size_t l = SDL_strlen(p);
			if( p[l-1] == '\\' ){
				//SDL_Log( "new subfolder: %s\n", p );
				SDL_snprintf( buffer, bufflen, "%s%s", folderpath, p );
				if( !SDL_EnumerateDirectory( buffer, enudir_callback, list ) ){
					SDL_Log("SDL_EnumerateDirectory (2) error: %s", SDL_GetError());
					SDL_Log(">{%s}", buffer );
				}
				ok_vec_remove( list, p );
				SDL_free( p );
				new_subdirs += 1;
			}
		}
		if( new_subdirs <= 0 ) break;
		depth -= 1;
	}

	// find where we are in the directory
	for (int i = 0; i < ok_vec_count( list ); ++i){
		if( strrcmp( pfname, ok_vec_get( list, i ) ) == 0 ){
			INDEX = i;
			break;
		}
	}
}



//=================================== /File io
//=================================== Image

void regularize_surface( SDL_Surface **S, SDL_PixelFormat target ){

	if( (*S)->format != target ){
		//SDL_Log("diff formats!! converting...");
		SDL_Surface *neo = SDL_ConvertSurface( *S, target );
		if( neo == NULL ) SDL_Log( "failed to convert: \"%s\"", SDL_GetError() );
		SDL_DestroySurface( *S );
		*S = neo;
	}
}


SDL_GPUShader* LoadShader( SDL_GPUDevice* device, const char* shaderFilename ){

	// Auto-detect the shader stage from the file name for convenience
	SDL_GPUShaderStage stage;
	if (SDL_strstr(shaderFilename, ".vert")){
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(shaderFilename, ".frag")){
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else if (SDL_strstr(shaderFilename, ".comp")){
		SDL_Log("compute.........");
		return NULL;
	}
	else{
		SDL_Log("Invalid shader stage!");
		return NULL;
	}
	
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
	const char *entrypoint = "main";

	void* code = NULL;
	size_t codesize = 0;

	const char* BasePath = SDL_GetBasePath();

	char src_path [256];
	SDL_snprintf(src_path, sizeof(src_path), "%sShaders\\Source\\%s.hlsl", BasePath, shaderFilename);
	SDL_PathInfo src_info = {0};
	bool src_exists = SDL_GetPathInfo( src_path, &src_info );

	// Check if we have it already compiled.
	char comp_path [256];
	SDL_snprintf(comp_path, sizeof(comp_path), "%sShaders\\Compiled\\SPIRV\\%s.spv", BasePath, shaderFilename);
	SDL_PathInfo comp_info = {0};
	bool comp_exists = SDL_GetPathInfo( comp_path, &comp_info );


	if( comp_exists && ( !src_exists || (src_exists && comp_info.modify_time >= src_info.modify_time) ) ){
		SDL_Log( "Loading pre-compiled [%s]", shaderFilename );
		code = SDL_LoadFile(comp_path, &codesize);
		if( code == NULL && src_exists ) goto compile_it;
	}
	// If we don't, compile it
	else{
		compile_it:;
		
		void* src_code = SDL_LoadFile(src_path, &codesize);

		if (src_code == NULL){
			SDL_Log("Failed to load shader [%s] from disk: %s", shaderFilename, SDL_GetError() );
			return NULL;
		}

		SDL_Log( "Compiling [%s]...", shaderFilename );

		SDL_ShaderCross_HLSL_Info hlsl_info = (SDL_ShaderCross_HLSL_Info){
			.source = src_code,	    /**< The HLSL source code for the shader. */
			.entrypoint = "main",	/**< The entry point function name for the shader in UTF-8. */
			.include_dir = NULL,	/**< The include directory for shader code. Optional, can be NULL. */
			.defines = NULL,		/**< An array of defines. Optional, can be NULL. If not NULL, must be terminated with a fully NULL define struct. */
			.shader_stage = stage,  /**< The shader stage to compile the shader with. */
			.props = 0			    /**< A properties ID for extensions. Should be 0 if no extensions are needed. */
		};
		code = SDL_ShaderCross_CompileSPIRVFromHLSL( &hlsl_info, &codesize );
		SDL_free( src_code );

		if( code == NULL ){
			SDL_Log("Failed to compile shader [%s]: %s", shaderFilename, SDL_GetError() );
			return NULL;
		}

		SDL_IOStream *cf = SDL_IOFromFile( comp_path, "wb" );
		if( SDL_WriteIO( cf, code, codesize) < codesize ){
			SDL_Log("Failed to save compiled shader [%s]: %s", shaderFilename, SDL_GetError() );
		}
		SDL_CloseIO( cf );
	}

	// Use reflection to automatically determine resource counts
	SDL_ShaderCross_GraphicsShaderMetadata* metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(code, codesize, 0);
	
	Uint32 samplerCount = 0;
	Uint32 uniformBufferCount = 0;
	Uint32 storageBufferCount = 0;
	Uint32 storageTextureCount = 0;
	
	if (metadata != NULL) {
		// Use reflection data to get resource counts
		SDL_ShaderCross_GraphicsShaderResourceInfo* resource_info = &metadata->resource_info;
		
		samplerCount = resource_info->num_samplers;
		uniformBufferCount = resource_info->num_uniform_buffers;
		storageBufferCount = resource_info->num_storage_buffers;
		storageTextureCount = resource_info->num_storage_textures;
		
		/*SDL_Log("Shader [%s] reflection: %u samplers, %u uniform buffers, %u storage buffers, %u storage textures", 
				shaderFilename, samplerCount, uniformBufferCount, 
				storageBufferCount, storageTextureCount);
		*/
	} else {
		SDL_Log("Warning: Failed to reflect shader [%s], using default resource counts of 0", shaderFilename);
	}

	SDL_GPUShaderCreateInfo shaderInfo = {
		.code = code,
		.code_size = codesize,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = samplerCount,
		.num_uniform_buffers = uniformBufferCount,
		.num_storage_buffers = storageBufferCount,
		.num_storage_textures = storageTextureCount
	};
	SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);

	SDL_free( code );
	if( metadata != NULL ){
		SDL_free( metadata );
	}
	if( shader == NULL ){
		SDL_Log("Failed to create shader: %s", SDL_GetError() );
		return NULL;
	}
	else return shader;
}


SDL_GPUComputePipeline* CreateComputePipelineFromShader( SDL_GPUDevice* device, const char* shaderFilename ){

	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
	const char *entrypoint = "main";

	Uint8 *bytecode = NULL;
	size_t bytecode_size = 0;

	const char* BasePath = SDL_GetBasePath();

	char src_path [256];
	SDL_snprintf(src_path, sizeof(src_path), "%sShaders\\Source\\%s.hlsl", BasePath, shaderFilename);
	SDL_PathInfo src_info = {0};
	bool src_exists = SDL_GetPathInfo( src_path, &src_info );

	// Check if we have it already compiled.
	char comp_path [256];
	SDL_snprintf(comp_path, sizeof(comp_path), "%sShaders\\Compiled\\SPIRV\\%s.spv", BasePath, shaderFilename);
	SDL_PathInfo comp_info = {0};
	bool comp_exists = SDL_GetPathInfo( comp_path, &comp_info );


	if( comp_exists && ( !src_exists || (src_exists && comp_info.modify_time >= src_info.modify_time) ) ){
		SDL_Log( "Loading pre-compiled [%s]", shaderFilename );
		bytecode = SDL_LoadFile(comp_path, &bytecode_size);
		if( bytecode == NULL && src_exists ) goto compile_it;
	}
	// If we don't, compile it
	else{
		compile_it:;
		
		size_t src_codesize = 0;
		Uint8* src_code = SDL_LoadFile(src_path, &src_codesize);

		if (src_code == NULL){
			SDL_Log("Failed to load shader [%s] from disk: %s", shaderFilename, SDL_GetError() );
			return NULL;
		}

		SDL_Log( "Compiling [%s]...", shaderFilename );

		SDL_ShaderCross_HLSL_Info hlsl_info = (SDL_ShaderCross_HLSL_Info){
			.source = src_code,	    /**< The HLSL source code for the shader. */
			.entrypoint = "main",	/**< The entry point function name for the shader in UTF-8. */
			.include_dir = NULL,	/**< The include directory for shader code. Optional, can be NULL. */
			.defines = NULL,		/**< An array of defines. Optional, can be NULL. If not NULL, must be terminated with a fully NULL define struct. */
			.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE,  /**< The shader stage to compile the shader with. */
			.props = 0			    /**< A properties ID for extensions. Should be 0 if no extensions are needed. */
		};
		bytecode = SDL_ShaderCross_CompileSPIRVFromHLSL( &hlsl_info, &bytecode_size );
		SDL_free( src_code );

		if( bytecode == NULL ){
			SDL_Log("Failed to compile shader [%s]: %s", shaderFilename, SDL_GetError() );
			return NULL;
		}

		SDL_IOStream *cf = SDL_IOFromFile( comp_path, "wb" );
		if( SDL_WriteIO( cf, bytecode, bytecode_size) < bytecode_size ){
			SDL_Log("Failed to save compiled shader [%s]: %s", shaderFilename, SDL_GetError() );
		}
		SDL_CloseIO( cf );
	}

	SDL_ShaderCross_ComputePipelineMetadata *metadata = SDL_ShaderCross_ReflectComputeSPIRV( bytecode,
																							 bytecode_size,
																							 0 );

	SDL_ShaderCross_SPIRV_Info spirv_info = {
		.bytecode = bytecode,              /**< The SPIRV bytecode. */
		.bytecode_size = bytecode_size,    /**< The length of the SPIRV bytecode. */
		.entrypoint = "main",              /**< The entry point function name for the shader in UTF-8. */
		.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE,  /**< The shader stage to transpile the shader with. */
	};

	SDL_GPUComputePipeline* pipeline = SDL_ShaderCross_CompileComputePipelineFromSPIRV( device, 
																						&spirv_info, 
																						metadata, 0 );

	SDL_free( metadata );
	SDL_free( bytecode );

	return pipeline;
}


typedef struct image_struct{

	int type;

	union{

		SDL_GPUTexture *TEXTURE;

		struct {
			SDL_GPUTexture *ORIGINAL;
			SDL_GPUTexture *SnBd;// SCALED n BLURRED
			float blur_zoom_threshhold;
		} B;// Big image

		struct {
			SDL_GPUTexture **TEXTURES;
			int framecount;
			int FRAME;
			Uint64 NFRAME;
			int *delays;
		} A; //ANIMATION

	} U;

	SDL_Rect RCT;
	SDL_GPUBuffer* vertex_buffer;

} Image;

enum image_type { INVALID = 0, SIMPLE, BIG, ANIMATION };


void process_animation( Image *img, IMG_Animation *ANIM ){
	img->type = ANIMATION;

	img->RCT = (SDL_Rect){ 0, 0, ANIM->frames[0]->w, ANIM->frames[0]->h };

	int scalemode = SDL_SCALEMODE_LINEAR;
	if( img->RCT.w <= 256 || img->RCT.h <= 256 ){
		antialiasing = 1;//SDL_SCALEMODE_NEAREST;//SDL_SCALEMODE_PIXELART;
	}
	//SDL_SetTextureScaleMode( img->U.A.TEXTURES[f], antialiasing );
	SDL_GPUTransferBufferCreateInfo quadbuf_CI = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = (sizeof(tvert) * 4)
	};
	SDL_GPUTransferBuffer* quad_buffer = SDL_CreateGPUTransferBuffer( device, &quadbuf_CI );

	tvert* dat = SDL_MapGPUTransferBuffer( device, quad_buffer, false );
	dat[0] = (tvert) { 0,                  0,                  0, 0, 0 };
	dat[1] = (tvert) { ANIM->frames[0]->w, 0,                  0, 1, 0 };
	dat[2] = (tvert) { ANIM->frames[0]->w, ANIM->frames[0]->h, 0, 1, 1 };
	dat[3] = (tvert) { 0,                  ANIM->frames[0]->h, 0, 0, 1 };
	SDL_UnmapGPUTransferBuffer(device, quad_buffer);

	SDL_GPUBufferCreateInfo vertbuf_CI = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(tvert) * 4
	};
	img->vertex_buffer = SDL_CreateGPUBuffer( device, &vertbuf_CI );
	//SDL_SetGPUBufferName( device, img->vertex_buffer, "texture vertex buffer" );
	//SDL_Log("uploading quad to GPU...");
	SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(device);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdbuf);

	SDL_GPUTransferBufferLocation location_V = {
		.transfer_buffer = quad_buffer,
		.offset = 0
	};
	SDL_GPUBufferRegion region_V = {
		.buffer = img->vertex_buffer,
		.offset = 0,
		.size = sizeof(tvert) * 4
	};
	SDL_UploadToGPUBuffer( copyPass, &location_V, &region_V, false );

	SDL_EndGPUCopyPass( copyPass );
	SDL_SubmitGPUCommandBuffer( cmdbuf );
	SDL_ReleaseGPUTransferBuffer(device, quad_buffer);

	img->U.A.framecount = ANIM->count;
	img->U.A.TEXTURES = SDL_malloc( img->U.A.framecount * sizeof( SDL_GPUTexture* ) );
	for (int f = 0; f < img->U.A.framecount; ++f ){
		//img->U.A.TEXTURES[f] = SDL_CreateTextureFromSurface( R, ANIM->frames[f] );
		
		regularize_surface( ANIM->frames + f, SDL_PIXELFORMAT_ABGR8888 );

		SDL_GPUTextureCreateInfo texture_CI = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.width  = ANIM->frames[f]->w,
			.height = ANIM->frames[f]->h,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
		};
		img->U.A.TEXTURES[f] = SDL_CreateGPUTexture(device, &texture_CI);
		//SDL_SetGPUTextureName( device, img->U.A.TEXTURES[f], path );

		SDL_GPUTransferBufferCreateInfo TTB_CI = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = ANIM->frames[f]->w * ANIM->frames[f]->h * 4
		};
		SDL_GPUTransferBuffer* texture_buffer = SDL_CreateGPUTransferBuffer( device, &TTB_CI );

		Uint8* textureTransferPtr = SDL_MapGPUTransferBuffer( device, texture_buffer, false );
		SDL_memcpy(textureTransferPtr, ANIM->frames[f]->pixels, ANIM->frames[f]->w * ANIM->frames[f]->h * 4);
		SDL_UnmapGPUTransferBuffer(device, texture_buffer);

		//SDL_Log("uploading texture to GPU...");
		cmdbuf = SDL_AcquireGPUCommandBuffer(device);
		copyPass = SDL_BeginGPUCopyPass(cmdbuf);

		SDL_GPUTextureTransferInfo transfer_info_T = {
			.transfer_buffer = texture_buffer,
			.offset = 0, /* Zeros out the rest */
		};
		SDL_GPUTextureRegion region_T = {
			.texture = img->U.A.TEXTURES[f],
			.w = ANIM->frames[f]->w,
			.h = ANIM->frames[f]->h,
			.d = 1
		};
		SDL_UploadToGPUTexture( copyPass, &transfer_info_T, &region_T, false );

		SDL_EndGPUCopyPass( copyPass );
		SDL_SubmitGPUCommandBuffer( cmdbuf );
		//SDL_DestroySurface( ANIM->frames[f] );
		SDL_ReleaseGPUTransferBuffer(device, texture_buffer);
		//SDL_Log("done!");
	}
	img->U.A.delays = SDL_malloc( img->U.A.framecount * sizeof( int ) );
	SDL_memcpy( img->U.A.delays, ANIM->delays, img->U.A.framecount * sizeof( int ) );
	img->U.A.FRAME = 0;
	img->U.A.NFRAME = SDL_GetTicks() + ANIM->delays[0];
	img->RCT = (SDL_Rect){ 0, 0, ANIM->w, ANIM->h };
}

void process_big_img( Image *img ){

	SDL_Log("processing a big'un!");

	img->type = BIG;
	img->U.B.SnBd = NULL;

	SDL_FRect target = (SDL_FRect){ 0,0,img->RCT.w, img->RCT.h };

	if( img->RCT.w > 2*width || img->RCT.h > 2*height ){
		SDL_Rect double_window = (SDL_Rect){ 0,0, 2*width, 2*height };
		fit_rect( &target, &double_window );
	}

	img->U.B.blur_zoom_threshhold = 1.333 * ( height / target.h );
	SDL_Log( "img->U.B.blur_zoom_threshhold = %g", img->U.B.blur_zoom_threshhold );


	img->U.B.SnBd = SDL_CreateGPUTexture( device, &(SDL_GPUTextureCreateInfo ){
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .width = target.w,
        .height = target.h,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE
    });
    SDL_Log( "Created a %gx%g texture, original is %dx%d", target.w, target.h, img->RCT.w, img->RCT.h );

	SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(device);
	if (cmdbuf == NULL){
		SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
		return;
	}

	SDL_GPUComputePass *computePass = SDL_BeginGPUComputePass(
		cmdbuf,
		&(SDL_GPUStorageTextureReadWriteBinding){
			.texture = img->U.B.SnBd,
			.layer = 0,
			.mip_level = 0,
			.cycle = true
		},
		1,
		NULL,
		0);

	SDL_BindGPUComputePipeline(computePass, blur_pipeline);
	SDL_BindGPUComputeSamplers(
		computePass,
		0,
		&(SDL_GPUTextureSamplerBinding){
			.texture = img->U.B.ORIGINAL,
			.sampler = sampler[1]
		},
		1);
	
	blur_uniforms BUs = {
		.input_w = img->RCT.w,
		.input_h = img->RCT.h,
    	.output_w = target.w,
    	.output_h = target.h,
    };
	SDL_PushGPUComputeUniformData(cmdbuf, 0, &BUs, sizeof(blur_uniforms));

	SDL_DispatchGPUCompute(computePass, (target.w +7) / 8, (target.h +7) / 8, 1);
	SDL_EndGPUComputePass(computePass);

	SDL_SubmitGPUCommandBuffer(cmdbuf);

	SDL_Log( "Done!");
}


void animation_tick( Image *img ){

	Uint64 now = SDL_GetTicks();
	if( now >= img->U.A.NFRAME ){
		img->U.A.FRAME = cycle( img->U.A.FRAME + 1, 0, img->U.A.framecount );
		img->U.A.NFRAME = now + img->U.A.delays[ img->U.A.FRAME ];
	}
}

void destroy_Image( Image *img ){

	switch( img->type ){

		case SIMPLE:
			if( img->U.TEXTURE != NULL ){
				SDL_ReleaseGPUTexture( device, img->U.TEXTURE );
				img->U.TEXTURE = NULL;
			}
			break;

		case BIG:
			SDL_ReleaseGPUTexture( device, img->U.B.ORIGINAL ); img->U.B.ORIGINAL = NULL;
			SDL_ReleaseGPUTexture( device, img->U.B.SnBd ); img->U.B.SnBd = NULL;
			break;

		case ANIMATION:
			for (int f = 0; f < img->U.A.framecount; ++f ){
				SDL_ReleaseGPUTexture( device, img->U.A.TEXTURES[f] );
				img->U.A.TEXTURES[f] = NULL;
			}
			SDL_free( img->U.A.TEXTURES );
			img->U.A.TEXTURES = NULL;
			SDL_free( img->U.A.delays );
			img->U.A.delays = NULL;
			break;
	}

	SDL_ReleaseGPUBuffer(device, img->vertex_buffer);
	img->type = INVALID;
}

/* modes:
0 - if it fits, centralize, else force-fit
1 - centralize
2 - force fit
*/
void calc_transform( Transform *T, SDL_Rect *RCT, int mode ){

	if( mode == 1 || ( mode == 0 && (RCT->w < width && RCT->h < height)) ){// CENTRALIZE IT 
		T->tx = 0.5 * ( width  - RCT->w);
		T->ty = 0.5 * ( height - RCT->h);
		T->scale_i = 0;
		T->scale = 1;
	}
	else{//                               DOESN'T FIT... FIT IT!
		SDL_FRect DST = (SDL_FRect){ RCT->x, RCT->y, RCT->w, RCT->h };
		fit_rect( &DST, &window_rect );
		T->tx = DST.x; T->ty = DST.y;
		T->scale = DST.w / (float) RCT->w;
		T->scale_i = logarithm( 1.1, T->scale );
		fit = 1;
	}
}


bool intersecting_or_touching( SDL_Rect *A, SDL_Rect *B ){
	return ( ( A->x + A->w >= B->x ) && ( B->x + B->w >= A->x ) ) && 
		   ( ( A->y + A->h >= B->y ) && ( B->y + B->h >= A->y ) );
}

typedef struct i2d_struct{ int i, j; } i2d;

// returns the total dimensions
i2d pack_imgs( Image *imgs, int len ){

	#define rct(i) imgs[i].RCT

	int *ids = SDL_malloc( len * sizeof(int) );
	for(int i = 0; i < len; i++) ids[i] = i;

	float targetAR = width / (float)height;

	while(1){
		bool done = 1;
		for(int i = len-1; i > 0; i--){
			if( (rct( ids[i] ).w > rct( ids[i-1] ).w && rct( ids[i] ).w > rct( ids[i-1] ).h) ||
				(rct( ids[i] ).h > rct( ids[i-1] ).w && rct( ids[i] ).h > rct( ids[i-1] ).h) ){

				int temp = ids[i-1];
				ids[i-1] = ids[i];
				ids[i] = temp;
				done = 0;
			}
		}
		if( done ) break;
	}

	int anchor_size = 2 * len;
	i2d *anchors = SDL_malloc( anchor_size * sizeof(i2d) );

	int tW = rct( ids[0] ).w;
	int tH = rct( ids[0] ).h;

	rct( ids[0] ).x = 0;
	rct( ids[0] ).y = 0;
	anchors[0] = (i2d){ tW+1, 0 };
	anchors[1] = (i2d){ 0, tH+1 };
	int anchor_len = 2;

	for(int i = 1; i < len; i++){
		
		int A = -1;
		float best = 999999;

		for(int a = 0; a < anchor_len; a++){

			SDL_Rect candidate = (SDL_Rect){ anchors[a].i, anchors[a].j, rct(ids[i]).w, rct(ids[i]).h };

			bool ouch = 0;
			for(int j = 0; j < i; j++){
				if( intersecting_or_touching( &candidate, &(imgs[ ids[j] ].RCT) ) ){
					ouch = 1;
					break;
				}
			}
			if( ouch ) continue;
			
			int right = anchors[a].i + rct(ids[i]).w;
			int bottom = anchors[a].j + rct(ids[i]).h;
			int nw = (right > tW)? right : tW;
			int nh = (bottom > tH)? bottom : tH;
			if( nw == tW && nh == tH ){
				A = a;
				break;
			}
			else{
				float AR = nw / (float) nh;
				float S = SDL_fabsf( targetAR - AR );
				if( S < best ){
					A = a;
					best = S;
				}
			}
		}

		rct( ids[i] ).x = anchors[A].i;
		rct( ids[i] ).y = anchors[A].j;
		int right = anchors[A].i + rct(ids[i]).w;
		int bottom = anchors[A].j + rct(ids[i]).h;
		anchors[A] = (i2d){ rct( ids[i] ).x, bottom + 1 };
		anchors[ anchor_len++ ] = (i2d){ right + 1, rct( ids[i] ).y };
		if(right > tW) tW = right;
		if(bottom > tH) tH = bottom;
	}

	// upload the new packed locations to the imgs' vertex buffers
	for(int i = 0; i < len; i++){
		SDL_GPUTransferBufferCreateInfo quadbuf_CI = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (sizeof(tvert) * 4)
		};
		SDL_GPUTransferBuffer* quad_buffer = SDL_CreateGPUTransferBuffer( device, &quadbuf_CI );

		tvert* dat = SDL_MapGPUTransferBuffer( device, quad_buffer, false );
		dat[0] = (tvert) { rct(i).x,            rct(i).y,            0, 0, 0 };
		dat[1] = (tvert) { rct(i).x + rct(i).w, rct(i).y,            0, 1, 0 };
		dat[2] = (tvert) { rct(i).x + rct(i).w, rct(i).y + rct(i).h, 0, 1, 1 };
		dat[3] = (tvert) { rct(i).x,            rct(i).y + rct(i).h, 0, 0, 1 };
		SDL_UnmapGPUTransferBuffer(device, quad_buffer);

		SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdbuf);

		SDL_GPUTransferBufferLocation location = {
			.transfer_buffer = quad_buffer,
			.offset = 0
		};
		SDL_GPUBufferRegion region_V = {
			.buffer = imgs[i].vertex_buffer,
			.offset = 0,
			.size = sizeof(tvert) * 4
		};
		SDL_UploadToGPUBuffer( copyPass, &location, &region_V, false );

		SDL_EndGPUCopyPass( copyPass );
		SDL_SubmitGPUCommandBuffer( cmdbuf );
		SDL_ReleaseGPUTransferBuffer(device, quad_buffer);
	}

	SDL_free( anchors );
	SDL_free( ids );


	return (i2d){ tW, tH };
	#undef rct
}


bool palette_func(void* closure, const char* name, int length, plutovg_color_t* color){
	*color = PLUTOVG_MAKE_COLOR(5,5,5,255);
	return true;
}


Image *IMAGES = NULL;
int IMAGES_N = 0;

int load_image( char *path, Image *out ){


	int EXT = check_extension( path );

	if( !EXT ) return 0;

	destroy_Image( out );

	unsigned char* pixels = NULL;
	SDL_Surface *SURF = NULL;

	if( EXT == 4 || EXT == 9 ){//.gif or webp
		IMG_Animation *ANIM = IMG_LoadAnimation( path );

		if( ANIM == NULL ){
			SDL_Log("bad anim, %s\n", SDL_GetError() );
			goto loadsimple;
		}
		else{
			if( ANIM->count == 1 ){
				SDL_Log( "1 frame anim" );
				SURF = SDL_DuplicateSurface( ANIM->frames[0] );
				out->type = SIMPLE;
			}
			else{
				SDL_Log( "good anim, %d frames\n", ANIM->count );
				process_animation( out, ANIM );
				animating = 1;
			}
			IMG_FreeAnimation( ANIM );
		}
	}
	else if( EXT == 10 ){//.svg

		plutosvg_document_t* doc = plutosvg_document_load_from_file( path, 1, 1 );//width, height
		if (!doc) {
			SDL_Log( "Failed to load SVG file: %s\n", path );
			return 0;
		}
		plutovg_rect_t bounds;
		plutosvg_document_extents( doc, NULL, &bounds );
		//SDL_Log( "\n%g,%g,%g,%g", bounds.x, bounds.y, bounds.w, bounds.h );

		int stride = bounds.w * 4; // Guaranteed 32-bit ARGB (4 bytes per pixel)
		pixels = SDL_calloc(bounds.h, stride);

		plutovg_surface_t* psurf = plutovg_surface_create_for_data( pixels, bounds.w, bounds.h, stride );
		//plutovg_surface_t *surface = plutovg_surface_create(width, height);
		plutovg_canvas_t *canvas = plutovg_canvas_create( psurf );
		plutovg_canvas_translate( canvas, -bounds.x, -bounds.y );

		plutovg_color_t currentc = PLUTOVG_MAKE_COLOR(5,5,5,255);
		bool result = plutosvg_document_render( doc, NULL, canvas, 
												&currentc, palette_func, NULL );
		
		//unsigned char* pixels = plutovg_surface_get_data( surface );
		//int pitch = SDL_ceil(bounds.w) * 4;
		//int stride = plutovg_surface_get_stride( surface );
		//SDL_Log( "pixels: %p, pitch: %d, stride: %d", pixels, pitch, stride );

		SURF = SDL_CreateSurfaceFrom( bounds.w, bounds.h, 
									  SDL_PIXELFORMAT_ARGB8888, 
									  pixels, stride );
		//IMG_SavePNG( SURF, "output.png" );
		if( SURF == NULL ){
			SDL_Log( "ERROR converting plutosvg surf to SDL: %s", SDL_GetError() );
		}
		else{
			out->type = SIMPLE;
		}
		plutovg_canvas_destroy(canvas);  
		plutovg_surface_destroy(psurf);
		plutosvg_document_destroy(doc);  
	}
	else{
		loadsimple:
		SURF = IMG_Load( path );
		out->type = SIMPLE;
	}

	if( out->type == INVALID || (out->type == SIMPLE && SURF == NULL) ){
		SDL_Log("bad image: %s\n", SDL_GetError());
		return 0;
	}
	
	if( out->type == SIMPLE ){
		out->RCT = (SDL_Rect){ 0, 0, SURF->w, SURF->h };

		int scalemode = SDL_SCALEMODE_LINEAR;
		if( out->RCT.w <= 256 || out->RCT.h <= 256 ){
			antialiasing = 1;//SDL_SCALEMODE_NEAREST;//SDL_SCALEMODE_PIXELART;
		}
		//SDL_SetTextureScaleMode( out->U.TEXTURE, antialiasing );

		regularize_surface( &SURF,  SDL_PIXELFORMAT_ABGR8888 );


		SDL_GPUTransferBufferCreateInfo quadbuf_CI = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = (sizeof(tvert) * 4)
		};
		SDL_GPUTransferBuffer* quad_buffer = SDL_CreateGPUTransferBuffer( device, &quadbuf_CI );


		tvert* dat = SDL_MapGPUTransferBuffer( device, quad_buffer, false );
		dat[0] = (tvert) { 0,       0,       0, 0, 0 };
		dat[1] = (tvert) { SURF->w, 0,       0, 1, 0 };
		dat[2] = (tvert) { SURF->w, SURF->h, 0, 1, 1 };
		dat[3] = (tvert) { 0,       SURF->h, 0, 0, 1 };
		SDL_UnmapGPUTransferBuffer(device, quad_buffer);

		SDL_GPUTextureCreateInfo texture_CI = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.width  = SURF->w,
			.height = SURF->h,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
		};
		out->U.TEXTURE = SDL_CreateGPUTexture(device, &texture_CI);
		SDL_SetGPUTextureName( device, out->U.TEXTURE, path );

		SDL_GPUBufferCreateInfo vertbuf_CI = {
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = sizeof(tvert) * 4
		};
		out->vertex_buffer = SDL_CreateGPUBuffer( device, &vertbuf_CI );
		//SDL_SetGPUBufferName( device, out->vertex_buffer, "texture vertex buffer" );

		SDL_GPUTransferBufferCreateInfo TTB_CI = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = SURF->w * SURF->h * 4
		};
		SDL_GPUTransferBuffer* texture_buffer = SDL_CreateGPUTransferBuffer( device, &TTB_CI );

		Uint8* textureTransferPtr = SDL_MapGPUTransferBuffer( device, texture_buffer, false );
		SDL_memcpy(textureTransferPtr, SURF->pixels, SURF->w * SURF->h * 4);
		SDL_UnmapGPUTransferBuffer(device, texture_buffer);


		SDL_Log("uploading texture to GPU...");
		SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdbuf);


		SDL_GPUTransferBufferLocation location = {
			.transfer_buffer = quad_buffer,
			.offset = 0
		};
		SDL_GPUBufferRegion region_V = {
			.buffer = out->vertex_buffer,
			.offset = 0,
			.size = sizeof(tvert) * 4
		};
		SDL_UploadToGPUBuffer( copyPass, &location, &region_V, false );

		SDL_GPUTextureTransferInfo transfer_info_T = {
			.transfer_buffer = texture_buffer,
			.offset = 0, /* Zeros out the rest */
		};
		SDL_GPUTextureRegion region_T = {
			.texture = out->U.TEXTURE,
			.w = SURF->w,
			.h = SURF->h,
			.d = 1
		};
		SDL_UploadToGPUTexture( copyPass, &transfer_info_T, &region_T, false );

		SDL_EndGPUCopyPass( copyPass );
		SDL_SubmitGPUCommandBuffer( cmdbuf );
		SDL_DestroySurface( SURF );
		if( pixels != NULL ) SDL_free(pixels);
		SDL_ReleaseGPUTransferBuffer(device, texture_buffer);
		SDL_ReleaseGPUTransferBuffer(device, quad_buffer);
		SDL_Log("done!");
		//SDL_Log( "But did it actually work? {%s}", SDL_GetError() );

		if( out->RCT.w > 2*width || out->RCT.h > 2*height ){
			process_big_img( out );
		}
	}	

	return 1;
}




//=================================== /Image


void FlexQuad_RenderPass( SDL_GPUCommandBuffer* cmdbuf, SDL_GPUColorTargetInfo *CTI, 
					      wireframe_uniforms *WUs ){
	SDL_GPURenderPass* RP = SDL_BeginGPURenderPass( cmdbuf, CTI, 1, NULL );
	SDL_BindGPUGraphicsPipeline( RP, wireframe_pipeline );

	SDL_PushGPUVertexUniformData( cmdbuf, 0, WUs, sizeof(wireframe_uniforms) );

	SDL_Time ticks = 0;
	SDL_GetCurrentTime( &ticks );
	SDL_DateTime dt;
	SDL_TimeToDateTime( ticks, &dt, true );
	float ns = dt.second * 0.0166666666666666666666666; //SDL_NS_TO_SECONDS( (float)(dt.nanosecond) );
	SDL_PushGPUFragmentUniformData( cmdbuf, 0, &ns, sizeof(float) );

	SDL_DrawGPUPrimitives( RP, FM_verts[ WUs->mode ], 1, 0, 0 );
	SDL_EndGPURenderPass( RP );
}

//=================================== Main

#define SWT_Loading() SDL_snprintf( buffer, bufflen, "Loading \"%s\"...  [%d / %lld]", \
									ok_vec_get(&directory_list, INDEX),                \
									INDEX+1, ok_vec_count( &directory_list ) );        \
					  SDL_SetWindowTitle( window, buffer );

#define SWT_img() SDL_snprintf( buffer, bufflen, "%s  •  (%d × %d)  •  [%d / %lld]",  \
								ok_vec_get(&directory_list, INDEX), W, H,             \
								INDEX+1, ok_vec_count( &directory_list ) );           \
				  SDL_SetWindowTitle( window, buffer );

#define SWT_imgs() SDL_snprintf( buffer, bufflen, "Introscopia's ImageViewer. %d images", IMAGES_N ); \
				   SDL_SetWindowTitle( window, buffer );

#define ADJUST_FOR_NEW_IMG() W = IMAGES[0].RCT.w;                            \
				   			 H = IMAGES[0].RCT.h;                            \
				   			 int mode = (fit || W < 100 || H < 100 )? 2 : 0; \
							 calc_transform( &T, &(IMAGES[0].RCT), mode );   \
							 SWT_img();                                      \
							 if( IMAGES[0].type == BIG ) enable_blur = true; \



SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv){


	if( !SDL_Init(SDL_INIT_VIDEO) ){
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
		return -1;
	}
	window = SDL_CreateWindow("ImageViewer", width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
	if (window == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "CreateWindow failed: %s", SDL_GetError());
		return -1;
	}
	SDL_GetWindowSize( window, &width, &height );
	aspect_correction = (height / (float)width);

	SDL_Init(SDL_INIT_VIDEO);

	SDL_SetHint( SDL_HINT_RENDER_VULKAN_DEBUG, "1" );

	window_rect = (SDL_Rect){0, 0, width, height};
	
	device = SDL_CreateGPUDevice( SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan" );
	if (device == NULL) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "CreateDevice failed"); return -1; }
	//SDL_Log( "device driver: %s", SDL_GetGPUDeviceDriver(device) );

	if (!SDL_ClaimWindowForGPUDevice(device, window)){
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ClaimWindow failed: %s", SDL_GetError());
		return -1;
	}

	SDL_GPUShader* vertexShader = LoadShader(device, "TexturedTransformedQuad.vert" ); //"TexturedQuad.vert" );
	if (vertexShader == NULL){ SDL_Log("Failed to create vertex shader!"); return -1; }

	SDL_GPUShader* fragmentShader = LoadShader(device, "TexturedQuad.frag" );
	if (fragmentShader == NULL){ SDL_Log("Failed to create fragment shader!"); return -1; }


	SDL_GPUGraphicsPipelineCreateInfo pipeline_CI = {
		.target_info = {
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(device, window),
				.blend_state = { 
					 .enable_blend = true,
					 .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					 .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					 .color_blend_op = SDL_GPU_BLENDOP_ADD,
					 .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					 .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					 .alpha_blend_op = SDL_GPU_BLENDOP_ADD
					}
			}},
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(tvert)
			}},
			.num_vertex_attributes = 2,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){{
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 0,
				.offset = 0
			}, {
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.location = 1,
				.offset = sizeof(float) * 3
			}}
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader
	};

	pipeline = NULL;
	pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_CI);
	if (pipeline == NULL) {
		SDL_Log("Failed to create pipeline!");
		return -1;
	}
	SDL_ReleaseGPUShader(device, vertexShader);
	SDL_ReleaseGPUShader(device, fragmentShader);


	SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	};
	sampler[0] = SDL_CreateGPUSampler(device, &sampler_info);

	sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
	sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
	sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;

	sampler[1] = SDL_CreateGPUSampler(device, &sampler_info);


	SDL_GPUBufferCreateInfo indbuf_CI = {
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = sizeof(Uint16) * 6
	};
	index_buffer = SDL_CreateGPUBuffer( device, &indbuf_CI );

	SDL_GPUTransferBufferCreateInfo indicesbuf_CI = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = (sizeof(Uint16) * 6)
	};
	SDL_GPUTransferBuffer* indices_buffer = SDL_CreateGPUTransferBuffer( device, &indicesbuf_CI );

	Uint16* indexData = SDL_MapGPUTransferBuffer( device, indices_buffer, false );
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 0;
	indexData[4] = 2;
	indexData[5] = 3;
	SDL_UnmapGPUTransferBuffer(device, indices_buffer);

	SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(device);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdbuf);
	SDL_GPUTransferBufferLocation indloc = {
		.transfer_buffer = indices_buffer,
		.offset = 0
	};
	SDL_GPUBufferRegion region_I = {
		.buffer = index_buffer,
		.offset = 0,
		.size = sizeof(Uint16) * 6
	};
	SDL_UploadToGPUBuffer( copyPass, &indloc, &region_I, false );

	SDL_EndGPUCopyPass( copyPass );
	SDL_SubmitGPUCommandBuffer( cmdbuf );
	SDL_ReleaseGPUTransferBuffer(device, indices_buffer);

	blur_pipeline = CreateComputePipelineFromShader( device, "gaussian_blur.comp" );


	//================================================================================================


	SDL_GPUShader* FlexQuads_vertShader = LoadShader( device, "FlexQuads.vert" );
	if (FlexQuads_vertShader == NULL){
		SDL_Log("Failed to create vertex shader!");
		return -1;
	}

	SDL_GPUShader* cubehelix_fragShader = LoadShader( device, "cubehelix.frag" );
	if (cubehelix_fragShader == NULL){
		SDL_Log("Failed to create fragment shader!");
		return -1;
	}

	SDL_GPUGraphicsPipelineCreateInfo wireframe_pipeline_CI = {
		.target_info = {
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){
				(SDL_GPUColorTargetDescription){
				.format = SDL_GetGPUSwapchainTextureFormat(device, window),
				.blend_state = (SDL_GPUColorTargetBlendState){ .enable_blend = false, }
				}
			},
		},
		.vertex_input_state = {0},
		.depth_stencil_state = {0},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST,
		.vertex_shader   = FlexQuads_vertShader,
		.fragment_shader = cubehelix_fragShader,
		.rasterizer_state = (SDL_GPURasterizerState){
			.fill_mode = SDL_GPU_FILLMODE_LINE,
			.cull_mode = SDL_GPU_CULLMODE_NONE,
		}
	};

	wireframe_pipeline = SDL_CreateGPUGraphicsPipeline(device, &wireframe_pipeline_CI);
	if (wireframe_pipeline == NULL){
		SDL_Log("Failed to create wireframe_pipeline: %s", SDL_GetError() );
		return -1;
	}
	SDL_ReleaseGPUShader(device, FlexQuads_vertShader);
	SDL_ReleaseGPUShader(device, cubehelix_fragShader);


	if( argc >= 2 ){

		/*SDL_Log("argc: %d\n", argc );
		for (int i = 0; i < argc; ++i ){
			SDL_Log( "[%d]: %s", i, argv[i] );
		}
		SDL_Log("SDL_GetCurrentDirectory(): %s\n", SDL_GetCurrentDirectory() );*/

		char pfname [512];
		CP_ACP_to_UTF8( pfname, argv[1] );
		//SDL_Log("pfname: %s\n", pfname );
		//for( int c = 0; pfname[c] != '\0' && c < 512; c++ ){ SDL_Log("%08X ", pfname[c] ); }

		size_t len = SDL_strlen( argv[1] );
		for (int i = len; i >=0 ; --i){
			if( pfname[i] == '\\' ){
				folderpath_len = i+1;
				break;
			}
		}
		folderpath = substr( pfname, 0, folderpath_len );
		//SDL_Log("folderpath: %s\n", folderpath );

		if( SDL_strncmp( folderpath, SDL_GetCurrentDirectory(), len ) != 0 ){
			//SDL_Log( "REMOTE OPERATION!!" );
			remote_operation = true;
		}

		load_folderlist( &directory_list, pfname, 1 );

		IMAGES_N = argc-1;
		IMAGES = SDL_calloc( IMAGES_N, sizeof(Image) );
		
		int is = 0;
		for (int i = 1; i < argc; ++i ){
			SWT_Loading();
			is = load_image( pfname, IMAGES + i-1 );

			if( i < argc-1 ) CP_ACP_to_UTF8( pfname, argv[i+1] );
		}
		
		if( argc > 2 ){
			i2d total = pack_imgs( IMAGES, IMAGES_N );
			W = total.i; H = total.j;
			SDL_Rect box = (SDL_Rect){0,0,W,H};
			calc_transform( &T, &box, 0 );
			SWT_imgs();
		} else if( is ) {
			ADJUST_FOR_NEW_IMG();
		}
	}

	sel_rect = (SDL_FRect){0};
	angle_i = 0;
	ANGLE = 0;
	FLIP = SDL_FLIP_NONE;
	cx = width / 2.0;
	cy = height / 2.0;
	mousePressed = 0;
	T = (Transform){0,0,1,0};
	mmpan = 0;// middle mouse pan
	zoom_in = 0;
	zoom_out = 0;
	pan_up = 0;
	pan_down = 0;
	pan_left = 0;
	pan_right = 0;
	rotate_ccw = 0;
	rotate_cw = 0;
	zoomV = 0.25;
	panV = 9;
	panVF = 0.04;//vel factor for the mmpan
	fullscreen = 0;
	//minimized = 0;


	return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){

	int psel = INDEX;
	int dir = 0;

	switch( event->type ){

		case SDL_EVENT_QUIT:
			return SDL_APP_SUCCESS;

		case SDL_EVENT_KEY_DOWN:

			switch( event->key.key ){
				case SDLK_LCTRL:
				case SDLK_RCTRL:
					CTRL  = 1;
					break;
				case SDLK_LSHIFT:
				case SDLK_RSHIFT:
					SHIFT = 1;
					break;

				case 'h':
				case SDLK_KP_4:
					pan_left = 1;
					break;
				case 'j':
				case SDLK_DOWN:
				case SDLK_KP_2:
					pan_down = 1;
					break;
				case 'k':
				case SDLK_UP:
				case SDLK_KP_8:
					pan_up = 1;
					break;
				case 'l':
				case SDLK_KP_6:
					pan_right = 1;
					break;

				case 'i':
				case SDLK_KP_5:
				case SDLK_KP_PLUS:
					zoom_in = 1; 
					break;
				case 'o':
				case SDLK_KP_0:
				case SDLK_KP_MINUS:
					zoom_out = 1;
					break;
			}
			update = 1;

			break;
		case SDL_EVENT_KEY_UP:;

			switch( event->key.key ){
				case SDLK_LCTRL:
				case SDLK_RCTRL:
					CTRL  = 0;
					break;

				case SDLK_LSHIFT:
				case SDLK_RSHIFT:
					SHIFT = 0;
					break;

				case SDLK_KP_7: rotate_ccw = 1; break;
				case SDLK_KP_9: rotate_cw = 1; break;
				case SDLK_KP_DIVIDE:
					if( FLIP & SDL_FLIP_HORIZONTAL ){
						FLIP &= ~SDL_FLIP_HORIZONTAL;
					}
					else FLIP |= SDL_FLIP_HORIZONTAL;
					break;
				case SDLK_KP_MULTIPLY:
					if( FLIP & SDL_FLIP_VERTICAL ){
						FLIP &= ~SDL_FLIP_VERTICAL;
					}
					else FLIP |= SDL_FLIP_VERTICAL;
					break;

				case SDLK_LEFT:
				case SDLK_KP_1:
					dir = -1; psel -= 1;
					break; 
				case SDLK_RIGHT:
				case SDLK_KP_3:
					dir =  1; psel -= 1;
					break;

				case 'h':
				case SDLK_KP_4:
					pan_left = 0;
					break;
				case 'j':
				case SDLK_DOWN:
				case SDLK_KP_2:
					pan_down = 0;
					break;
				case 'k':
				case SDLK_UP:
				case SDLK_KP_8:
					pan_up = 0;
					break;
				case 'l':
				case SDLK_KP_6:
					pan_right = 0;
					break;

				case 'i':
				case SDLK_KP_5:
				case SDLK_KP_PLUS:
					zoom_in = 0; 
					break;
				case 'o':
				case SDLK_KP_0:
				case SDLK_KP_MINUS:
					zoom_out = 0;
					break;

				case SDLK_SPACE:// FIT to WINDOW
					if( IMAGES_N == 1 ){
						calc_transform( &T, &(IMAGES[0].RCT), 2 );
					} else {
						SDL_Rect ALL = (SDL_Rect){ 0, 0, W, H };
						calc_transform( &T, &ALL, 2 );
					}
					fit = true;
					angle_i = 0;
					ANGLE = 0;
					FLIP = SDL_FLIP_NONE;
					break;

				case SDLK_0 ... SDLK_9: // SET ZOOM
					T.scale = event->key.key - '0';
					T.scale_i = logarithm( 1.1, T.scale );
					if( IMAGES[0].type == BIG && 
						T.scale > IMAGES[0].U.B.blur_zoom_threshhold ) enable_blur = false;
					T.tx = 0.5 * ( width  - (T.scale * W) );
					T.ty = 0.5 * ( height - (T.scale * H) );
					fit = 0;
					angle_i = 0;
					ANGLE = 0;
					FLIP = SDL_FLIP_NONE;
					break;

				case 'c':// BACKGROUND COLOR
					sel_bg--;
					if( sel_bg < 0 ) sel_bg = 4;
					break;

				case 'a':// ANTI_ALIASING
					antialiasing++;
					if( antialiasing > 1 ) antialiasing = 0;

					/*
					for (int i = 0; i < IMAGES_N; ++i ){
						SDL_SetTextureScaleMode( IMAGES[i].U.TEXTURE, antialiasing );
					}*/
					/**SDL_SCALEMODE_NEAREST,  < nearest pixel sampling */
					/**SDL_SCALEMODE_LINEAR,   < linear filtering */
					/**SDL_SCALEMODE_PIXELART  < nearest pixel sampling with improved scaling for pixel art */
					break;

				case 'b':// BLUR
					enable_blur = !enable_blur;
					break;

				case 's':{// SHUFFLE LIST
					char pfname [512];
					SDL_strlcpy( pfname, ok_vec_get( &directory_list, INDEX ), 512 );

					shuffle_str_list( ok_vec_begin(&directory_list), ok_vec_count(&directory_list) );

					// find where we are in the list
					for (int i = 0; i < ok_vec_count( &directory_list ); ++i){	
						if( strrcmp( pfname, ok_vec_get( &directory_list, i ) ) == 0 ){
							INDEX = i;
							break;
						}
					}
					} break;

				case SDLK_F5:{

					psel = -1;
					char pfname [512];
					SDL_strlcpy( pfname, ok_vec_get( &directory_list, INDEX ), 512 );

					destroy_str_vec( &directory_list );
					load_folderlist( &directory_list, pfname, 1 );
					} break;

				case SDLK_F6:{

					//psel = -1;
					char pfname [512];
					SDL_strlcpy( pfname, ok_vec_get( &directory_list, INDEX ), 512 );

					destroy_str_vec( &directory_list );
					load_folderlist( &directory_list, pfname, 9999 );

					SWT_img();
					} break;

				case SDLK_F11:
					if( fullscreen ){
						SDL_SetWindowFullscreen( window, false );
						fullscreen = 0;
					}
					else{
						//SDL_SetWindowFullscreen( window, SDL_WINDOW_FULLSCREEN_DESKTOP );
						SDL_SetWindowFullscreen( window, true );
						SDL_SetWindowFullscreenMode( window,  NULL );
						fullscreen = 1;
					}
					break;

				case SDLK_ESCAPE:
					if( fullscreen ){
						SDL_SetWindowFullscreen( window, 0 );
						//SDL_MaximizeWindow( window );
						fullscreen = 0;
					}
					break;

				case SDLK_DELETE:
					if( SHIFT ){
						SDL_RemovePath( ok_vec_get( &directory_list, INDEX ) );
						ok_vec_remove_at( &directory_list, INDEX );
						int dlc = ok_vec_count( &directory_list );
						if( dlc <= 0 ){
							return SDL_APP_SUCCESS;
						}
						if( INDEX >= dlc ) INDEX = dlc-1;
						psel--;
						dir = 0;
					}
					break;
			}
			update = 1;

			break;
		case SDL_EVENT_MOUSE_MOTION:

			pmouseX = mouseX;
			pmouseY = mouseY;
			mouseX = event->motion.x;
			mouseY = event->motion.y;
			if( mousePressed ){
				if( CTRL ){
					//int tcx = SDL_round(T.tx + (scale * clickX));
					//int tcy = SDL_round(T.ty + (scale * clickY));
					sel_rect.x = SDL_min( mouseX, clickX );
					sel_rect.y = SDL_min( mouseY, clickY );
					sel_rect.w = SDL_abs( mouseX - clickX );
					sel_rect.h = SDL_abs( mouseY - clickY );
				}
				else{
					T.tx += event->motion.xrel;
					T.ty += event->motion.yrel;
					fit = 0;
				}
				update = 1;
			}

			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:

			if( CTRL ){
				clickX = mouseX;//SDL_round( (mouseX - T.tx) / scale );
				clickY = mouseY;//SDL_round( (mouseY - T.ty) / scale );
				sel_rect.x = mouseX;//SDL_round( T.tx + (scale * clickX) );
				sel_rect.y = mouseY;//SDL_round( T.ty + (scale * clickY) );
				sel_rect.w = 1;
				sel_rect.h = 1;
			}

			if( event->button.button == SDL_BUTTON_MIDDLE ){
				mmpan = 1;
				clickX = mouseX;
				clickY = mouseY;
			}
			else if( event->button.button == SDL_BUTTON_X1 ){
				dir = -1; psel -= 1;
			} 
			else if( event->button.button == SDL_BUTTON_X2 ){
				dir = 1; psel -= 1;
			}
			else mousePressed = 1;
			update = 1;


			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:

			mousePressed = 0;

			if( CTRL ){
				
				SDL_FRect fitrect = (SDL_FRect){ 0, 0, sel_rect.w, sel_rect.h };
				fit_rect( &fitrect, &window_rect );

				float otx = (sel_rect.x - T.tx) / T.scale;
				float oty = (sel_rect.y - T.ty) / T.scale;
				T.scale *= (fitrect.w / (float) sel_rect.w);
				T.scale_i = logarithm( 1.1, T.scale );
				if( IMAGES[0].type == BIG && 
					T.scale > IMAGES[0].U.B.blur_zoom_threshhold ) enable_blur = false;
				T.tx = fitrect.x - (otx * T.scale);
				T.ty = fitrect.y - (oty * T.scale);
				clickX = -1;
			}
			if( event->button.button == SDL_BUTTON_MIDDLE ){
				mmpan = 0;
			}
			update = 1;

			break;
		case SDL_EVENT_MOUSE_WHEEL:;

			float mx = mouseX;
			float my = mouseY;
			float xrd = (mx - T.tx) / T.scale;
			float yrd = (my - T.ty) / T.scale;
			T.scale_i -= event->wheel.y;
			T.scale = SDL_pow( 1.1, T.scale_i );
			SDL_Log( "T.scale = %g", T.scale );
			if( IMAGES[0].type == BIG && 
				T.scale > IMAGES[0].U.B.blur_zoom_threshhold ) enable_blur = false;
			T.tx = mx - xrd * T.scale;
			T.ty = my - yrd * T.scale;

			fit = 0;
			update = 1;

			break;

		case SDL_EVENT_DROP_FILE:
		case SDL_EVENT_DROP_TEXT:
			{
			//printf("event->drop.source: %s\n", event->drop.source );
			//SDL_Log("event->drop.data: %s\n", event->drop.data );

			SDL_snprintf( buffer, bufflen, "Adding \"%s\" via drop...", event->drop.data );
			SDL_SetWindowTitle( window, buffer );

			int I = IMAGES_N;
			IMAGES_N += 1;
			IMAGES = SDL_realloc( IMAGES, IMAGES_N * sizeof(Image) );
			SDL_memset( IMAGES+I, 0, sizeof(Image) );
			//SDL_Log("loading %s",  event->drop.data );
			if( !load_image( event->drop.data, IMAGES + I ) ){
				IMAGES_N -= 1;
				IMAGES = SDL_realloc( IMAGES, IMAGES_N * sizeof(Image) );
			}
			else{
				//SDL_Log("success! now pack it.." );
				i2d total = pack_imgs( IMAGES, IMAGES_N );
				W = total.i; H = total.j;
				//SDL_Log("packed. W: %d, H: %d", W, H );
				SDL_Rect box = (SDL_Rect){0,0,W,H};
				calc_transform( &T, &box, 0 );
				SWT_imgs();
				update = 1;
			}

			} break;

		/*case SDL_EVENT_WINDOW_MINIMIZED:
			minimized = 1;
			break;*/

		case SDL_EVENT_WINDOW_RESTORED:
		case SDL_EVENT_WINDOW_RESIZED:
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		case SDL_EVENT_WINDOW_MAXIMIZED:
		case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
		case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:

			SDL_GetWindowSize( window, &width, &height );
			aspect_correction = (height / (float)width);
			window_rect.w = width;
			window_rect.h = height;
			cx = width / 2.0;
			cy = height / 2.0;
			SDL_Rect box = (SDL_Rect){0,0,W,H};
			int mode = fit ? 2 : 1;
			calc_transform( &T, &box, mode );
			update = 1;
		break;

	}

	if( psel != INDEX && IMAGES_N == 1 ){
		int count = 0;
		animating = 0;
		char path [1024];
		int is = 0;

		while( count < ok_vec_count( &directory_list ) ){

			INDEX = cycle( INDEX+dir, 0, ok_vec_count( &directory_list ) );
			//SDL_Log("INDEX[%d]: %s\n", INDEX, ok_vec_get( &directory_list, INDEX ) );

			if( remote_operation ){
				SDL_snprintf( path, 1024, "%s%s", folderpath, ok_vec_get( &directory_list, INDEX ) );
			} else {
				SDL_strlcpy( path, ok_vec_get( &directory_list, INDEX ), 1024 );
			}
			//SDL_Log("path: %s\n", path );

			//CP_ACP_to_UTF8( path, buffer ); // CP_ACP_to_UTF8( path );
			//SDL_Log("loading path: %s\n", path );

			SWT_Loading();
			is = load_image( path, IMAGES + 0 );
			if( is ) break;

			count++;
		}
		
		if( is == 1 ) ADJUST_FOR_NEW_IMG();
	}

	return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void *appstate){


	if( pan_up || pan_down || pan_left || pan_right ){
		if( pan_up    ) T.ty += panV;
		if( pan_down  ) T.ty -= panV;
		if( pan_left  ) T.tx += panV;
		if( pan_right ) T.tx -= panV;
		update = 1;
	}
	if( mmpan ){
		T.tx += panVF * (clickX - mouseX);
		T.ty += panVF * (clickY - mouseY);
		update = 1;
	}
	if( zoom_in || zoom_out ){
		float xrd = ( cx - T.tx) / T.scale;
		float yrd = ( cy - T.ty) / T.scale;
		if( zoom_in  ) T.scale_i += zoomV;
		if( zoom_out ) T.scale_i -= zoomV;
		T.scale = SDL_pow( 1.1, T.scale_i );
		if( IMAGES[0].type == BIG && 
			T.scale > IMAGES[0].U.B.blur_zoom_threshhold ) enable_blur = false;
		T.tx =  cx - xrd * T.scale;
		T.ty =  cy - yrd * T.scale;
		fit = 0;
		update = 1;
	}
	if( rotate_cw || rotate_ccw ){
		angle_i += rotate_cw - rotate_ccw;
		ANGLE = HALF_PI * (angle_i % 4);
		rotate_cw = 0;
		rotate_ccw = 0;
		update = 1;

		if( IMAGES_N > 1 ){
			// lot's a work
		}
	}

	if( update || animating ){
		update = 0;

		SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(device);
		if (cmdbuf == NULL){
			SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
			return -1;
		}
		SDL_GPUTexture* swapchainTexture;
		if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, window, &swapchainTexture, NULL, NULL)) {
			SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
			return -1;
		}

		if (swapchainTexture != NULL){

			SDL_GPUColorTargetInfo CTI = { 0 };
			CTI.texture = swapchainTexture; // to dest
			CTI.clear_color = bg[ sel_bg ];
			CTI.load_op = SDL_GPU_LOADOP_CLEAR;
			CTI.store_op = SDL_GPU_STOREOP_STORE;
			SDL_GPURenderPass* RP = SDL_BeginGPURenderPass(cmdbuf, &CTI, 1, NULL );
			SDL_EndGPURenderPass( RP );

			for (int i = 0; i < IMAGES_N; ++i ){

				CTI.load_op = SDL_GPU_LOADOP_LOAD;

				SDL_Rect dst = apply_transform_rect( &(IMAGES[i].RCT), &T );
				wireframe_uniforms WUs = {
					.window = (SDL_FPoint){width,height},
					.A = (SDL_FPoint){dst.x,dst.y},
					.B = (SDL_FPoint){dst.w,dst.h},
					.C = 12,
					.mode = FM_corners
				};
				FlexQuad_RenderPass( cmdbuf, &CTI, &WUs );

				SDL_GPUTexture *TEX = NULL;

				switch( IMAGES[i].type ){

					case SIMPLE:
						TEX = IMAGES[i].U.TEXTURE;
						break;

					case BIG:
						if( enable_blur && IMAGES[i].U.B.SnBd != NULL ){
							TEX = IMAGES[i].U.B.SnBd;
						} else {
							TEX = IMAGES[i].U.B.ORIGINAL;
						}
						break;

					case ANIMATION:
						animation_tick( IMAGES + i );
						TEX = IMAGES[i].U.A.TEXTURES[ IMAGES[i].U.A.FRAME ];
						break;
				}

				RP = SDL_BeginGPURenderPass(cmdbuf, &CTI, 1, NULL);
				
				SDL_GPUBufferBinding Vbinding = { .buffer = IMAGES[i].vertex_buffer, .offset = 0 };
				SDL_GPUBufferBinding Ibinding = { .buffer = index_buffer,            .offset = 0 };
				SDL_GPUTextureSamplerBinding TSB = { .texture = TEX, 
													 .sampler = sampler[ antialiasing ] };
				/*Transform NormT = (Transform){ map(T.tx, 0, width,  -1,  1), 
												 map(T.ty, 0, height,  1, -1), T.scale };*/
				UniDat uni;
				uni.transform = transformToMatrix4x4( &T, ANGLE, FLIP );
				//Log_M4x4( &tmat );
				uni.w = width; uni.h = height;

				SDL_BindGPUGraphicsPipeline( RP, pipeline );
				SDL_BindGPUVertexBuffers( RP, 0, &Vbinding, 1 );
				SDL_BindGPUIndexBuffer( RP, &Ibinding, SDL_GPU_INDEXELEMENTSIZE_16BIT );
				SDL_BindGPUFragmentSamplers( RP, 0, &TSB, 1 );
				SDL_PushGPUVertexUniformData( cmdbuf, 0, &uni, sizeof(UniDat) );
				SDL_DrawGPUIndexedPrimitives( RP, 6, 1, 0, 0, 0 );

				SDL_EndGPURenderPass( RP );
			}

			if( mmpan ){
				//SDL_RenderRect( R, &(SDL_FRect){clickX-6, clickY-6, 12, 12 } );
				wireframe_uniforms anchor = {
					.window = (SDL_FPoint){width,height},
					.A = (SDL_FPoint){clickX,clickY},
					.B = (SDL_FPoint){10,10},
					.C = 0,
					.mode = FM_diamond
				};
				FlexQuad_RenderPass( cmdbuf, &CTI, &anchor );

				for(float d = 0.25; d < 1; d += 0.25 ){
					//SDL_RenderRect( R, &(SDL_FRect){ x-3, y-3, 6, 6 } );
					wireframe_uniforms doots = {
						.window = (SDL_FPoint){width,height},
						.A = (SDL_FPoint){clickX + d * ( mouseX - clickX ),
										  clickY + d * ( mouseY - clickY )},
						.B = (SDL_FPoint){4,4},
						.C = 0,
						.mode = FM_diamond
					};
					FlexQuad_RenderPass( cmdbuf, &CTI, &doots );
				}
			}
			if( mousePressed && CTRL ){
				//SDL_RenderRect( R, &sel_rect );
				wireframe_uniforms sel_rct = {
					.window = (SDL_FPoint){width,height},
					.A = (SDL_FPoint){sel_rect.x, sel_rect.y},
					.B = (SDL_FPoint){sel_rect.w, sel_rect.h},
					.C = 0,
					.mode = FM_rect
				};
				FlexQuad_RenderPass( cmdbuf, &CTI, &sel_rct );
			}
		}

		SDL_SubmitGPUCommandBuffer(cmdbuf);
	}
	SDL_framerateDelay( 16 );
	return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result){

	SDL_Log("SDL_Quiting..");


	SDL_ReleaseGPUBuffer(device, index_buffer);
	SDL_ReleaseGPUSampler(device, sampler[0]);
	SDL_ReleaseGPUSampler(device, sampler[1]);
	
	for (int i = 0; i < IMAGES_N; ++i ){
		destroy_Image( IMAGES + i );
	}
	SDL_free( IMAGES );

	SDL_ReleaseGPUGraphicsPipeline(device, wireframe_pipeline);
	SDL_ReleaseGPUComputePipeline(device, blur_pipeline);

	SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
	SDL_DestroyGPUDevice(device);
	SDL_DestroyWindow(window);

	SDL_Log("BYE");
}