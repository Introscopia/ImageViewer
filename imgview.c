#include <SDL.h>
#include <SDL_image.h>
#include "ok_lib.h"

#define PLUTOSVG_BUILD_STATIC
#define PLUTOVG_BUILD_STATIC
#include <plutosvg/plutosvg.h>

#include <windows.h>

SDL_Renderer *R;
SDL_Window *window;
int width = 500, height = 500;// of the window
SDL_Rect window_rect;

Sint64 max_T_size;

#define bufflen 1024
char buffer [ bufflen ];

typedef struct ok_vec_of(const char *) str_vec;




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

// cycle indices "array style"
int cycle( int a, int min, int max ){
	if( a < min ) return max-1;
	else if( a >= max ) return min;
	else return a;
}

void Win_to_UTF8(char *output, const char* input) {

	int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);

	wchar_t* wideCharStr = SDL_malloc(wideCharLength * sizeof(wchar_t));

	MultiByteToWideChar(CP_UTF8, 0, input, -1, wideCharStr, wideCharLength);

	int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, NULL, 0, NULL, NULL);

	WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, output, utf8Length, NULL, NULL);

	SDL_free(wideCharStr);
	//return utf8Str;
}

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
	const char exts [][6] = { ".png", ".jpg", "jpeg", ".gif", ".tif", "tiff", ".ico", ".bmp", "webp", ".svg" };

	size_t len = SDL_strlen( filename );
	for (int i = 0; i < 10; ++i ){
		if( SDL_strcasecmp( filename + len -4, exts[i] ) == 0 ){
			return i+1;
		}
	}
	return 0;
}

/*
size_t cleanse_path( char *path ){
	const char windows_forbidden_chars [] = "<>\"|?*";//  ":/\\" are also forbidden chars, but...
	size_t len = SDL_strlen( path );
	for (int c = 0; path[c] != '\0'; ++c ){
		for (int i = 0; windows_forbidden_chars[i] != '\0'; ++i ){
			if( path[c] == windows_forbidden_chars[i] ){
				SDL_memmove( path+c, path+c+1, len-c );
				len -= 1;
			}	
		}
	}
	return len;
}*/

char *folderpath = NULL;
int folderpath_len = 0;
int INDEX = 0;// of the present file in the list
bool remote_operation = false;

int antialiasing = SDL_SCALEMODE_LINEAR;
bool fit = false;
bool animating = false;
int tasking = 0;
bool enable_blur = true;
float blur_zoom_threshhold = 0.18;



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



void regularize_surface( SDL_Surface **S, SDL_Surface *T ){

	if( (*S)->format != T->format ){
		SDL_Log("diff formats!! converting...");
		SDL_Surface *temp = SDL_ConvertSurface( *S, T->format );
		SDL_DestroySurface( *S );
		*S = temp;
	}
}


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


typedef struct transform_struct{
	float tx, ty; // translate
    float scale;
    float scale_i; // index, scale = 1.1 ^ scale_i
} Transform;

double logarithm( double base, double x ){
	return SDL_log10( x ) / SDL_log10( base );
}

SDL_FRect apply_transform_rect( SDL_Rect *rct, Transform *T ){
	return (SDL_FRect){ T->tx + (T->scale * rct->x), 
						T->ty + (T->scale * rct->y), 
					    rct->w * T->scale, rct->h * T->scale };
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


void draw_corners( SDL_Renderer*R, SDL_FRect *DST, int w ){
	// Top-left
	SDL_RenderLine(R, DST->x, DST->y, DST->x + w, DST->y);  
	SDL_RenderLine(R, DST->x, DST->y, DST->x, DST->y + w);  
	// Top-right
	SDL_RenderLine(R, DST->x + DST->w - 1 - w, DST->y, DST->x + DST->w - 1, DST->y);  
	SDL_RenderLine(R, DST->x + DST->w - 1, DST->y, DST->x + DST->w - 1, DST->y + w);  
	// Bottom-left
	SDL_RenderLine(R, DST->x, DST->y + DST->h - 1, DST->x + w, DST->y + DST->h - 1);  
	SDL_RenderLine(R, DST->x, DST->y + DST->h - 1 - w, DST->x, DST->y + DST->h - 1);  
	// Bottom-right
	SDL_RenderLine(R, DST->x + DST->w - 1 - w, DST->y + DST->h - 1, DST->x + DST->w - 1, DST->y + DST->h - 1);
	SDL_RenderLine(R, DST->x + DST->w - 1, DST->y + DST->h - 1 - w, DST->x + DST->w - 1, DST->y + DST->h - 1);  
}


// Gaussian function for weights
static inline float gaussian(float x, float sigma) {
    return SDL_expf(-(x * x) / (2.0f * sigma * sigma)) / (SDL_sqrtf(2 * SDL_PI_F) * sigma);
}

SDL_Surface* load_scale_n_blur( const char* filepath, int target_w, int target_h, float blur){
    // Load image
    SDL_Surface* original = IMG_Load(filepath);
    if (!original) {
        SDL_Log("Failed to load image: %s", SDL_GetError());
        return NULL;
    }

    SDL_Surface* converted = SDL_ConvertSurface(original, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(original);
    if (!converted) {
        SDL_Log("Failed to convert surface: %s", SDL_GetError());
        return NULL;
    }

    SDL_FRect crct = (SDL_FRect){0,0,converted->w, converted->h};
    SDL_Rect trct = (SDL_Rect){0,0,target_w, target_h};
    fit_rect( &crct, &trct );
    target_w = crct.w;
    target_h = crct.h;

    // Create target surface
    SDL_Surface* output = SDL_CreateSurface(target_w, target_h, converted->format);
    if (!output) {
        SDL_Log("Failed to create surface: %s", SDL_GetError());
        SDL_DestroySurface(converted);
        return NULL;
    }
    
    float scale = (float)converted->w / target_w;
    float sigma = scale * 0.5f;
    int radius = SDL_ceilf(sigma * blur);
    int lens_len = (2 * radius + 1) * (2 * radius + 1);
    
    float* lens = SDL_malloc( lens_len * sizeof(float) );
    float sum = 0.0f;
    
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            float dist = SDL_sqrtf(x*x + y*y);
            float weight = gaussian(dist, sigma);
            lens[(y + radius) * (2 * radius + 1) + (x + radius)] = weight;
            sum += weight;
        }
    }
    
    // Normalize
    sum = 1.0 / sum;
    for (int i = 0; i < lens_len; i++) {
        lens[i] *= sum;
    }

    SDL_PixelFormatDetails *deets = SDL_GetPixelFormatDetails(output->format);

    SDL_LockSurface(converted);
    SDL_LockSurface(output);

    Uint32* src_pixels = (Uint32*)converted->pixels;
    Uint32* dst_pixels = (Uint32*)output->pixels;

    for (int dst_y = 0; dst_y < target_h; dst_y++) {
        for (int dst_x = 0; dst_x < target_w; dst_x++) {
            float src_center_x = dst_x * scale;
            float src_center_y = dst_y * scale;
            
            float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
            
            for (int ky = -radius; ky <= radius; ky++) {
                for (int kx = -radius; kx <= radius; kx++) {
                    int src_x = (int)(src_center_x + kx);
                    int src_y = (int)(src_center_y + ky);
                    
                    src_x = SDL_clamp(src_x, 0, converted->w - 1);
                    src_y = SDL_clamp(src_y, 0, converted->h - 1);
                    
                    Uint32 pixel = src_pixels[src_y * converted->w + src_x];
                    SDL_Color color;
                    SDL_GetRGBA(pixel, deets, NULL, &color.r, &color.g, &color.b, &color.a);
                    
                    float weight = lens[(ky + radius) * (2 * radius + 1) + (kx + radius)];
                    r += color.r * weight;
                    g += color.g * weight;
                    b += color.b * weight;
                    a += color.a * weight;
                }
            }
            
            dst_pixels[dst_y * target_w + dst_x] = SDL_MapRGBA(
                deets, NULL,
                (uint8_t)SDL_clamp(r, 0.0f, 255.0f),
                (uint8_t)SDL_clamp(g, 0.0f, 255.0f),
                (uint8_t)SDL_clamp(b, 0.0f, 255.0f),
                (uint8_t)SDL_clamp(a, 0.0f, 255.0f)
            );
        }
    }

    SDL_UnlockSurface(converted);
    SDL_UnlockSurface(output);
    SDL_free(lens);

    // Cleanup
    SDL_DestroySurface(converted);

    return output;
}


typedef struct {
    SDL_Mutex* lock;
    SDL_Thread* thread;
    char filepath[1024];
    int target_w, target_h;
    float blur_factor;
    SDL_Surface* output;
    int progress;
    int completed;
    int cancel_requested;
} BigImg_LSnB_Task;

int LSnB_thread(void* data) {
    BigImg_LSnB_Task* task = (BigImg_LSnB_Task*)data;
    
    SDL_Surface* surf = load_scale_n_blur( task->filepath, 
                                           task->target_w, task->target_h, 
                                           task->blur_factor );
    
    SDL_LockMutex( task->lock );
    if( task->cancel_requested ){
    	if (surf) {
	        SDL_DestroySurface(surf);
	    }
	}else{
        task->output = surf;
        task->completed = 1;
    }
    SDL_UnlockMutex( task->lock );
    
    return 0;
}

BigImg_LSnB_Task* launch_LSnB_thread( const char* filepath, int w, int h, float blur) {

    BigImg_LSnB_Task* task = SDL_calloc( 1, sizeof(BigImg_LSnB_Task) );
    *task = (BigImg_LSnB_Task){
        .lock = SDL_CreateMutex(),
        .target_w = w,
        .target_h = h,
        .blur_factor = blur
    };
    SDL_strlcpy(task->filepath, filepath, sizeof(task->filepath));
    
    task->thread = SDL_CreateThread( LSnB_thread, "load_scale_n_blur_big_image", task );
    return task;
}

SDL_Texture* check_LSnB_Task( SDL_Renderer *R, BigImg_LSnB_Task* task ){
    SDL_LockMutex( task->lock );
    SDL_Texture* result = NULL;
    if (task->completed) {
		result = SDL_CreateTextureFromSurface( R, task->output );
		if (!result) {
		    SDL_Log("Failed to create texture: %s", SDL_GetError());
		}
		SDL_DestroySurface( task->output );
        task->output = NULL;
        task->completed = 0;
    }
    SDL_UnlockMutex(task->lock);
    return result;
}

void cancel_and_destroy_task( BigImg_LSnB_Task* task ){
    SDL_LockMutex( task->lock );
    task->cancel_requested = 1;
    if( task->output ){
        SDL_DestroySurface( task->output );
        task->output = NULL;
    }
    SDL_UnlockMutex( task->lock );
    
    SDL_WaitThread( task->thread, NULL );
    SDL_DestroyMutex( task->lock );
    SDL_free( task );
}



typedef struct image_struct{

	int type;

	union{

		SDL_Texture *TEXTURE;

		struct {
			SDL_Texture *ORIGINAL;
			SDL_Texture *SCALEDnBLURRED;
			BigImg_LSnB_Task *task;
		} B;// Big image

		struct {
			SDL_Texture **TEXTURES;
			int framecount;
			int FRAME, NFRAME;
			int *delays;
		} A;// Animation

	} U;

	SDL_Rect RCT;

} Image;

enum image_type { INVALID = 0, SIMPLE, BIG, ANIMATION };

void process_animation( Image *img, IMG_Animation *ANIM ){
	img->type = ANIMATION;
	img->U.A.framecount = ANIM->count;
	img->U.A.TEXTURES = SDL_malloc( img->U.A.framecount * sizeof( SDL_Texture* ) );
	for (int f = 0; f < img->U.A.framecount; ++f ){
		img->U.A.TEXTURES[f] = SDL_CreateTextureFromSurface( R, ANIM->frames[f] );
	}
	img->U.A.delays = SDL_malloc( img->U.A.framecount * sizeof( int ) );
	SDL_memcpy( img->U.A.delays, ANIM->delays, img->U.A.framecount * sizeof( int ) );
	img->U.A.FRAME = 0;
	img->U.A.NFRAME = SDL_GetTicks() + ANIM->delays[0];
	img->RCT = (SDL_Rect){ 0, 0, ANIM->w, ANIM->h };
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
			SDL_DestroyTexture( img->U.TEXTURE );
			img->U.TEXTURE = NULL;
			break;

		case BIG:
			SDL_DestroyTexture( img->U.B.ORIGINAL );
			SDL_DestroyTexture( img->U.B.SCALEDnBLURRED );
			if( img->U.B.task ){
				cancel_and_destroy_task( img->U.B.task );
			}
			break;

		case ANIMATION:
			for (int f = 0; f < img->U.A.framecount; ++f ){
				SDL_DestroyTexture( img->U.A.TEXTURES[f] );
				img->U.A.TEXTURES[f] = NULL;
			}
			SDL_free( img->U.A.TEXTURES );
			img->U.A.TEXTURES = NULL;
			SDL_free( img->U.A.delays );
			img->U.A.delays = NULL;
			break;
	}
	img->type = INVALID;
}

/* modes:
0 - if it fits, centralize, else force-fit
1 - centralize
2 - force fit
*/
void calc_transform( Transform *T, SDL_Rect *RCT, int mode ){

	if( mode == 1 || ( mode == 0 && (RCT->w < width && RCT->h < height)) ){// CENTRALIZE IT 
		T->tx = 0.5 * (width  - RCT->w);
		T->ty = 0.5 * (height - RCT->h);
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

	#define rects(i) imgs[i].RCT

	int *ids = SDL_malloc( len * sizeof(int) );
	for(int i = 0; i < len; i++) ids[i] = i;

	float targetAR = width / (float)height;

	while(1){
		bool done = 1;
		for(int i = len-1; i > 0; i--){
			if( (rects( ids[i] ).w > rects( ids[i-1] ).w && rects( ids[i] ).w > rects( ids[i-1] ).h) ||
				(rects( ids[i] ).h > rects( ids[i-1] ).w && rects( ids[i] ).h > rects( ids[i-1] ).h) ){

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

	int W = rects( ids[0] ).w;
	int H = rects( ids[0] ).h;

	rects( ids[0] ).x = 0;
	rects( ids[0] ).y = 0;
	anchors[0] = (i2d){ W+1, 0 };
	anchors[1] = (i2d){ 0, H+1 };
	int anchor_len = 2;

	for(int i = 1; i < len; i++){
		
		int A = -1;
		float best = 999999;

		for(int a = 0; a < anchor_len; a++){

			SDL_Rect candidate = (SDL_Rect){ anchors[a].i, anchors[a].j, rects(ids[i]).w, rects(ids[i]).h };

			bool ouch = 0;
			for(int j = 0; j < i; j++){
				if( intersecting_or_touching( &candidate, &(imgs[ ids[j] ].RCT) ) ){
					ouch = 1;
					break;
				}
			}
			if( ouch ) continue;
			
			int right = anchors[a].i + rects(ids[i]).w;
			int bottom = anchors[a].j + rects(ids[i]).h;
			int nw = (right > W)? right : W;
			int nh = (bottom > H)? bottom : H;
			if( nw == W && nh == H ){
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

		rects( ids[i] ).x = anchors[A].i;
		rects( ids[i] ).y = anchors[A].j;
		int right = anchors[A].i + rects(ids[i]).w;
		int bottom = anchors[A].j + rects(ids[i]).h;
		anchors[A] = (i2d){ rects( ids[i] ).x, bottom + 1 };
		anchors[ anchor_len++ ] = (i2d){ right + 1, rects( ids[i] ).y };
		if(right > W) W = right;
		if(bottom > H) H = bottom;
	}

	SDL_free( anchors );
	SDL_free( ids );


	return (i2d){ W, H };
	#undef rects
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

	if( EXT == 4 || EXT == 9 ){//.gif or webp
		IMG_Animation *ANIM = IMG_LoadAnimation( path );

		if( ANIM == NULL ){
			SDL_Log("bad anim, %s\n", SDL_GetError() );
			goto loadtexture;
		}
		else{
			if( ANIM->count == 1 ){
				out->U.TEXTURE = SDL_CreateTextureFromSurface( R,  ANIM->frames[0] );
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
		unsigned char* pixels = SDL_calloc(bounds.h, stride);

		plutovg_surface_t* surface = plutovg_surface_create_for_data(pixels, bounds.w, bounds.h, stride);
		//plutovg_surface_t *surface = plutovg_surface_create(width, height);
		plutovg_canvas_t *canvas = plutovg_canvas_create( surface );
		plutovg_canvas_translate( canvas, -bounds.x, -bounds.y );

		plutovg_color_t currentc = PLUTOVG_MAKE_COLOR(5,5,5,255);
		bool result = plutosvg_document_render( doc, NULL, canvas, 
		                                        &currentc, palette_func, NULL);
		
		//unsigned char* pixels = plutovg_surface_get_data( surface );
		//int pitch = SDL_ceil(bounds.w) * 4;
		//int stride = plutovg_surface_get_stride( surface );
		//SDL_Log( "pixels: %p, pitch: %d, stride: %d", pixels, pitch, stride );

		SDL_Surface* sdl_surface = SDL_CreateSurfaceFrom( bounds.w, bounds.h, 
														  SDL_PIXELFORMAT_ARGB8888, 
														  pixels, stride );
		//IMG_SavePNG( sdl_surface, "output.png" );
		if( sdl_surface == NULL ){
			SDL_Log( "ERROR converting plutosvg surf to SDL: %s", SDL_GetError() );
		}
		else{
		    out->U.TEXTURE = SDL_CreateTextureFromSurface( R, sdl_surface );
		    out->type = SIMPLE;
		}

	    SDL_DestroySurface(sdl_surface); 
	    plutovg_canvas_destroy(canvas);  
	    plutovg_surface_destroy(surface);
    	plutosvg_document_destroy(doc);  
	}
	else{
		loadtexture:
		out->U.TEXTURE = IMG_LoadTexture( R, path );
		out->type = SIMPLE;
	}

	if( out->type == INVALID || (out->type == SIMPLE && out->U.TEXTURE == NULL) ){
		SDL_Log("bad texture: %s\n", SDL_GetError());
		
		SDL_Surface *SURF = IMG_Load( path );

		if( SURF == NULL ){
			SDL_Log( "bad surface" );
			SDL_snprintf( buffer, bufflen, "ERROR: %s", SDL_GetError() );
			SDL_SetWindowTitle( window, buffer );
			return 0;
		}
		else{
			/* This feature is cool.... but SDL can't actually process big images at all!
			if( SURF->w > max_T_size || SURF->h > max_T_size ){ // Too big.... break it up into a grid
				int gnx = SDL_ceilf( SURF->w / (float)max_T_size );
				int gny = SDL_ceilf( SURF->h / (float)max_T_size );
				int gw = SDL_ceilf( SURF->w / (float)gnx );
				int gh = SDL_ceilf( SURF->h / (float)gny );
				//SDL_Log( "breaking in up into a %dx%d of %dx%d px each.\n", gnx, gny, gw, gh );

				if( out != IMAGES ){
					SDL_snprintf( buffer, bufflen, "\"%s\" is too large to be opened in a group. Open it by itself.", path );
				  	SDL_SetWindowTitle( window, buffer );
				  	return 0;
				}

				IMAGES_N = gnx * gny;
				IMAGES = SDL_realloc( IMAGES, IMAGES_N * sizeof(Image) );
				SDL_memset( IMAGES, 0, IMAGES_N * sizeof(Image) );
				int I = 0;

				SDL_Surface *bufsurf = SDL_CreateSurface( gw, gh, SURF->format );
				SDL_Rect src = (SDL_Rect){ 0, 0, gw, gh };

				for (int j = 0; j < gny; ++j ){
					for (int i = 0; i < gnx; ++i ){
						SDL_BlitSurface( SURF, &src, bufsurf, NULL );
						IMAGES[I].type = SIMPLE;
						IMAGES[I].U.TEXTURE = SDL_CreateTextureFromSurface( R, bufsurf );
						IMAGES[I].RCT.x = src.x;
						IMAGES[I].RCT.y = src.y;
						I++;
						src.x += gw;
					}
					src.y += gh;
				}

				return 2;
			}
			else{*/

			out->U.TEXTURE = SDL_CreateTextureFromSurface( R, SURF );
			out->type = SIMPLE;
			SDL_DestroySurface( SURF );
		}
	}
	
	if( out->type == SIMPLE ){
		float fw, fh;
		SDL_GetTextureSize( out->U.TEXTURE, &fw, &fh );
		out->RCT = (SDL_Rect){ 0, 0, fw, fh };

		if( antialiasing > 0 && (out->RCT.w <= 256 || out->RCT.h <= 256) ){
			antialiasing = SDL_SCALEMODE_NEAREST;//SDL_SCALEMODE_PIXELART;
			SDL_SetTextureScaleMode( out->U.TEXTURE, antialiasing );
		}

		if( (fw > width || fh > height) && EXT != 10 ){
			out->type = BIG;
			out->U.B.SCALEDnBLURRED = NULL;
			//float xs = width / fw;
			//float ys = height / fh;
			//out->U.B.zoom_threshhold =  //SDL_min( xs, ys );
			out->U.B.task = launch_LSnB_thread( path, width, height, 1.25 );
			tasking += 1;
		}
	}

	return 1;
}


#define SWT_Loading() SDL_snprintf( buffer, bufflen, "Loading \"%s\"...  [%d / %d]", \
									ok_vec_get(&directory_list, INDEX),              \
									INDEX, ok_vec_count( &directory_list ) );        \
					  SDL_SetWindowTitle( window, buffer );

#define SWT_img() SDL_snprintf( buffer, bufflen, "%s  •  (%d × %d)  •  [%d / %d]",  \
					            ok_vec_get(&directory_list, INDEX), W, H,           \
					            INDEX, ok_vec_count( &directory_list ) );           \
				  SDL_SetWindowTitle( window, buffer );

#define SWT_imgs() SDL_snprintf( buffer, bufflen, "Introscopia's ImageViewer. %d images", IMAGES_N ); \
				   SDL_SetWindowTitle( window, buffer );


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~O~~~~~~~~~~| M A I N |~~~~~~~~~~~O~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int main(int argc, char *argv[]){

	bool loop = 1;

	bool CTRL = 0;
	bool SHIFT = 0;

	int mouseX, mouseY, pmouseX, pmouseY;

	if( !SDL_Init(SDL_INIT_VIDEO) ) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
		return 3;
	}
	if( !SDL_CreateWindowAndRenderer( "Introscopia's ImageViewer built with SDL3", 
		                              width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED, 
		                              &window, &R) ){
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError() );
		return 3;
	}
	//SDL_MaximizeWindow( window );
	SDL_GetWindowSize( window, &width, &height );

	SDL_PropertiesID RPID = SDL_GetRendererProperties( R );
	max_T_size = SDL_GetNumberProperty( RPID, SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0);


	SDL_srand(0);


	SDL_Color bg [] = { {0, 0, 0, 255},
						{85, 85, 85, 255},
						{127, 127, 127, 255},
						{171, 171, 171, 255},
						{255, 255, 255, 255} };
	int sel_bg = 1;



	

	int W, H;//total width and height of the contents onscreen
	window_rect = (SDL_Rect){0, 0, width, height};
	SDL_Rect max_window_rect = (SDL_Rect){0, 0, width, height};
	SDL_FRect sel_rect = (SDL_FRect){0,0,0,0};
	int angle_i = 0;
	double ANGLE = 0;
	SDL_FlipMode FLIP = SDL_FLIP_NONE;
	

	float cx = width / 2.0;
	float cy = height / 2.0;
	bool mousePressed = 0;
	int clickX, clickY;
	
	Transform T = (Transform){0,0,1,0};
	
	bool mmpan = 0;// middle mouse pan
	bool zoom_in = 0;
	bool zoom_out = 0;
	bool pan_up = 0;
	bool pan_down = 0;
	bool pan_left = 0;
	bool pan_right = 0;
	bool rotate_ccw = 0;
	bool rotate_cw = 0;

	float zoomV = 0.25;
	float panV = 9;
	float panVF = 0.04;//vel factor for the mmpan
	
	bool fullscreen = 0;
	bool minimized = 0;

	

	str_vec directory_list;

	bool KONTINUOUS = false;


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
		/*
			if( is == 2 ){// img which got split into a grid
				W = IMAGES[ IMAGES_N-1 ].RCT.x + IMAGES[ IMAGES_N-1 ].RCT.w;
				H = IMAGES[ IMAGES_N-1 ].RCT.y + IMAGES[ IMAGES_N-1 ].RCT.h;
				SDL_Rect box = (SDL_Rect){0,0,W,H};
				calc_transform( &T, &box, 0 );
				SDL_snprintf( buffer, bufflen, "%s  •  (%d × %d)  •  [%d / %d]", pfname, W, H, INDEX, ok_vec_count( &directory_list ) );
				SDL_SetWindowTitle( window, buffer );
			} else if( is == 1 ){
			*/
		
		if( argc > 2 ){
			i2d total = pack_imgs( IMAGES, IMAGES_N );
			W = total.i; H = total.j;
			SDL_Rect box = (SDL_Rect){0,0,W,H};
			calc_transform( &T, &box, 0 );
			SWT_imgs();
		} else if( is ) {
			W = IMAGES[0].RCT.w; H = IMAGES[0].RCT.h;
			calc_transform( &T, &(IMAGES[0].RCT), 0 );
			SWT_img();
		}
	}


	SDL_SetRenderDrawColor( R, bg[sel_bg].r, bg[sel_bg].g, bg[sel_bg].b, bg[sel_bg].a );
	SDL_RenderClear( R );
	SDL_RenderPresent( R );

	int first = 3;

	//SDL_Log("<<Entering Loop>>");
	while ( loop ) { //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> L O O P <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< 

		int update = first;
		if( first > 0 ) first--;

		SDL_Event event;
		while( SDL_PollEvent(&event) ){

			int psel = INDEX;
			int dir = 0;

			switch (event.type) {
				case SDL_EVENT_QUIT:

					loop = 0;

					break;
				case SDL_EVENT_KEY_DOWN:

					switch( event.key.key ){
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

					switch( event.key.key ){
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
							angle_i = 0;
							ANGLE = 0;
							FLIP = SDL_FLIP_NONE;
							break;

						case SDLK_0 ... SDLK_9: // SET ZOOM
							T.scale = event.key.key - '0';
							T.scale_i = logarithm( 1.1, T.scale );
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

							for (int i = 0; i < IMAGES_N; ++i ){
								SDL_SetTextureScaleMode( IMAGES[i].U.TEXTURE, antialiasing );
							}
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
								char path [256];
								SDL_RemovePath( ok_vec_get( &directory_list, INDEX ) );
								//remove_item_from_string_list( &directory_list, INDEX, &list_len );
								ok_vec_remove_at( &directory_list, INDEX );
								//destroy_Image( IMAGES+0 );
								//INDEX++;
								psel--;
								dir = 1;
							}
							break;
					}
					update = 1;

					break;
				case SDL_EVENT_MOUSE_MOTION:

					pmouseX = mouseX;
					pmouseY = mouseY;
					mouseX = event.motion.x;
					mouseY = event.motion.y;
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
							T.tx += event.motion.xrel;
							T.ty += event.motion.yrel;
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

					if( event.button.button == SDL_BUTTON_MIDDLE ){
						mmpan = 1;
						clickX = mouseX;
						clickY = mouseY;
					}
					else if( event.button.button == SDL_BUTTON_X1 ){
						dir = -1; psel -= 1;
					} 
					else if( event.button.button == SDL_BUTTON_X2 ){
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
						T.tx = fitrect.x - (otx * T.scale);
						T.ty = fitrect.y - (oty * T.scale);
						clickX = -1;
					}
					if( event.button.button == SDL_BUTTON_MIDDLE ){
						mmpan = 0;
					}
					update = 1;

					break;
				case SDL_EVENT_MOUSE_WHEEL:;

					float xrd = (mouseX - T.tx) / T.scale;
					float yrd = (mouseY - T.ty) / T.scale;
					T.scale_i -= event.wheel.y;
					T.scale = SDL_pow( 1.1, T.scale_i );
					T.tx = mouseX - xrd * T.scale;
					T.ty = mouseY - yrd * T.scale;
					fit = 0;
					update = 1;

					break;

				case SDL_EVENT_DROP_FILE:
				case SDL_EVENT_DROP_TEXT:
					{
					//printf("event.drop.source: %s\n", event.drop.source );
					//SDL_Log("event.drop.data: %s\n", event.drop.data );

					SDL_snprintf( buffer, bufflen, "Adding \"%s\" via drop...", event.drop.data );
					SDL_SetWindowTitle( window, buffer );

					int I = IMAGES_N;
					IMAGES_N += 1;
					IMAGES = SDL_realloc( IMAGES, IMAGES_N * sizeof(Image) );
					SDL_memset( IMAGES+I, 0, sizeof(Image) );
					//SDL_Log("loading %s",  event.drop.data );
					if( !load_image( event.drop.data, IMAGES + I ) ){
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

				case SDL_EVENT_WINDOW_MINIMIZED:
					minimized = 1;
					break;

				case SDL_EVENT_WINDOW_RESTORED:
				case SDL_EVENT_WINDOW_RESIZED:
				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				case SDL_EVENT_WINDOW_MAXIMIZED:
				case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
				case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:

					SDL_GetWindowSize( window, &width, &height );
					window_rect.w = width;
					window_rect.h = height;
					cx = width / 2;
					cy = height / 2;
					SDL_Rect box = (SDL_Rect){0,0,W,H};
					calc_transform( &T, &box, 0 );
					update = 1;
				break;

			}

			if( psel != INDEX && !KONTINUOUS && IMAGES_N == 1 ){
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
				
				if( is == 2 ){
					W = IMAGES[ IMAGES_N-1 ].RCT.x + IMAGES[ IMAGES_N-1 ].RCT.w;
					H = IMAGES[ IMAGES_N-1 ].RCT.y + IMAGES[ IMAGES_N-1 ].RCT.h;
					SDL_Rect box = (SDL_Rect){0,0,W,H};
					calc_transform( &T, &box, 0 );
				} else if( is == 1 ) {
					W = IMAGES[0].RCT.w; H = IMAGES[0].RCT.h;
					calc_transform( &T, &(IMAGES[0].RCT), 0 );
				}
				SWT_img();
			}
		}

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
			float xrd = (cx - T.tx) / T.scale;
			float yrd = (cy - T.ty) / T.scale;
			if( zoom_in  ) T.scale_i += zoomV;
			if( zoom_out ) T.scale_i -= zoomV;
			T.scale = SDL_pow( 1.1, T.scale_i );
			T.tx = cx - xrd * T.scale;
			T.ty = cy - yrd * T.scale;
			fit = 0;
			update = 1;
		}
		if( rotate_cw || rotate_ccw ){
			angle_i += rotate_cw - rotate_ccw;
			ANGLE = 90 * (angle_i % 4);
			rotate_cw = 0;
			rotate_ccw = 0;
			update = 1;

			if( IMAGES_N > 1 ){
				// lot's a work
			}
		}

		if( update || animating || tasking ){

			SDL_SetRenderDrawColor( R, bg[sel_bg].r, bg[sel_bg].g, bg[sel_bg].b, bg[sel_bg].a );
			SDL_RenderClear( R );

			for (int i = 0; i < IMAGES_N; ++i ){

				SDL_Texture *TEX = NULL;
				SDL_FRect DST = apply_transform_rect( &(IMAGES[i].RCT), &T );

				switch( IMAGES[i].type ){

					case SIMPLE:
						TEX = IMAGES[i].U.TEXTURE;
						break;

					case BIG:
						if( IMAGES[i].U.B.task ){
							SDL_Texture *t = check_LSnB_Task( R, IMAGES[i].U.B.task );
							if( t != NULL ){
								IMAGES[i].U.B.SCALEDnBLURRED = t;
								cancel_and_destroy_task( IMAGES[i].U.B.task );
								IMAGES[i].U.B.task = NULL;
								tasking -= 1;
								if( enable_blur ){
									TEX = IMAGES[i].U.B.SCALEDnBLURRED;
								}
								else TEX =  IMAGES[i].U.B.ORIGINAL;
							}
							else TEX =  IMAGES[i].U.B.ORIGINAL;
						}
						else{
							if( enable_blur && T.scale < blur_zoom_threshhold ){
								TEX = IMAGES[i].U.B.SCALEDnBLURRED;
							} else {
								TEX = IMAGES[i].U.B.ORIGINAL;
							}
						}
						break;

					case ANIMATION:
						animation_tick( IMAGES + i );
						TEX = IMAGES[i].U.A.TEXTURES[ IMAGES[i].U.A.FRAME ];
						break;
				}

				//set a contrasting color
				if( sel_bg > 2 ) SDL_SetRenderDrawColor( R, bg[0].r, bg[0].g, bg[0].b, bg[0].a );
				else SDL_SetRenderDrawColor( R, bg[4].r, bg[4].g, bg[4].b, bg[4].a );
				draw_corners( R, &DST, 5 );

				if( angle_i != 0 || FLIP != SDL_FLIP_NONE ){
					SDL_RenderTextureRotated( R, TEX, NULL, &DST, ANGLE, NULL, FLIP );
				} else {
					SDL_RenderTexture( R, TEX, NULL, &DST );
				}
			}

			if( mmpan ){
				SDL_SetRenderDrawColor( R, 0, 255, 0, 255 );
				SDL_RenderRect( R, &(SDL_FRect){clickX-6, clickY-6, 12, 12 } );
				for(float d = 0.25; d < 1; d += 0.25 ){
					float x = clickX + d * ( mouseX - clickX );
					float y = clickY + d * ( mouseY - clickY );
					SDL_RenderRect( R, &(SDL_FRect){ x-3, y-3, 6, 6 } );
				}
			}
			if( mousePressed && CTRL ){
				SDL_SetRenderDrawColor( R, 0, 255, 0, 255 );
				SDL_RenderRect( R, &sel_rect );
			}


			SDL_RenderPresent( R );
		}

		SDL_framerateDelay( 16 );

	}//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> / L O O P <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	SDL_free( folderpath );
	destroy_str_vec( &directory_list );

	for (int i = 0; i < IMAGES_N; ++i ){
		destroy_Image( IMAGES + i );
	}
	SDL_free( IMAGES );

	SDL_DestroyRenderer( R );
	SDL_DestroyWindow( window );

	SDL_Quit();

	return 0;
}

