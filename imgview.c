#include "basics.h"
#include <SDL.h>
#include <SDL_image.h>
#include "ok_lib.h"

#include <windows.h>

#include <dirent.h>
#include <errno.h>
#include <locale.h>

#define bufflen 1024
char buffer [ bufflen ];

void destroy_str_vec( str_vec *v ){

	ok_vec_foreach_rev( v, char *str ) {
		free( str );
	}
	ok_vec_deinit( v );
}

//reverse cmp
int strrcmp( char *A, char *B ){
	int lA = strlen(A);
	int lB = strlen(B);
	int l = min( lA, lB );
	for (int i = 1; i < l; ++i ){
		int r = A[lA-i] - B[lB-i];
		if( r != 0 ) return r;
	}
	return 0;
}


void Win_to_UTF8(char *output, const char* input) {

	int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);

	wchar_t* wideCharStr = malloc(wideCharLength * sizeof(wchar_t));

	MultiByteToWideChar(CP_UTF8, 0, input, -1, wideCharStr, wideCharLength);

	int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, NULL, 0, NULL, NULL);

	//char* utf8Str = malloc( utf8Length );

	WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, output, utf8Length, NULL, NULL);

	free(wideCharStr);
	//return utf8Str;
}

char* Win_to_opendir(const char* input) {

	int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);

	wchar_t* wideCharStr = malloc(wideCharLength * sizeof(wchar_t));

	MultiByteToWideChar(CP_UTF8, 0, input, -1, wideCharStr, wideCharLength);

	//UTF-16 to the system's code page (e.g., CP_ACP or CP1252)
	int codePageLength = WideCharToMultiByte(CP_ACP, 0, wideCharStr, -1, NULL, 0, NULL, NULL);

	char* codePageStr = malloc(codePageLength);

	//from UTF-16 to the system's code page (CP_ACP)
	WideCharToMultiByte(CP_ACP, 0, wideCharStr, -1, codePageStr, codePageLength, NULL, NULL);

	free(wideCharStr);
	return codePageStr;
}

void CP_ACP_to_UTF8(char *output, const char* input) {
	// from the system's code page (e.g., CP_ACP) to UTF-16 (wide characters)
	int wideCharLength = MultiByteToWideChar(CP_ACP, 0, input, -1, NULL, 0);

	// Allocate memory for the wide-character string (UTF-16)
	wchar_t* wideCharStr = (wchar_t*)malloc(wideCharLength * sizeof(wchar_t));

	//to wide-char (UTF-16)
	MultiByteToWideChar(CP_ACP, 0, input, -1, wideCharStr, wideCharLength);

	//from UTF-16 to UTF-8
	int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, NULL, 0, NULL, NULL);

	//char* utf8Str = (char*)malloc(utf8Length);

	WideCharToMultiByte(CP_UTF8, 0, wideCharStr, -1, output, utf8Length, NULL, NULL);

	free(wideCharStr);
	//return utf8Str;
}

int check_extension( char *filename ){
	//                           1       2       3       4       5      6        7       8      9
	const char exts [][5] = { ".png", ".jpg", "jpeg", ".gif", ".tif", "tiff", ".ico", ".bmp", "webp" };

	int len = strlen( filename );
	for (int i = 0; i < 9; ++i ){
		if( SDL_strcasecmp( filename + len -4, exts[i] ) == 0 ){
			return i+1;
		}
	}
	return 0;
}

int folderpath_len = 0;

SDL_EnumerationResult enudir_callback(void *userdata, const char *dirname, const char *fname){

	int dl = strlen( dirname );
	if( dl > folderpath_len ){
		sprintf( buffer, "%s%s", dirname + folderpath_len, fname );
	}
	else{
		SDL_strlcpy( buffer, fname, bufflen );
	}
	size_t len = strlen( buffer );
	//printf(">buf:[%s]\n", buffer );

	if( check_extension( fname ) ){
		const char **neo = ok_vec_push_new( (str_vec*)userdata );
		*neo = calloc( len+1, sizeof(char) );
		memcpy( *neo, buffer, len+1 );
	}
	else{
		SDL_PathInfo info = {0};
		SDL_GetPathInfo( buffer, &info );
		if( info.type == SDL_PATHTYPE_DIRECTORY ){
			//puts("it's a dir!!");
			const char **neo = ok_vec_push_new( (str_vec*)userdata );
			*neo = calloc( len+2, sizeof(char) );
			sprintf( *neo, "%s\\", buffer );
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


void regularize_surface( SDL_Surface **S, SDL_Surface *T ){

	if( (*S)->format != T->format ){
		//puts("diff formats!! converting...");
		SDL_Surface *temp = SDL_ConvertSurface( *S, T->format );
		SDL_DestroySurface( *S );
		*S = temp;
	}
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~O~~~~~~~~~~| M A I N |~~~~~~~~~~~O~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int main(int argc, char *argv[]){

	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Event event;
	int width = 1;
	int height = 1;
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
		                              &window, &renderer) ){
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError() );
		return 3;
	}
	//SDL_MaximizeWindow( window );
	SDL_GetWindowSize( window, &width, &height );

	SDL_Surface *window_surf = SDL_GetWindowSurface( window );

	int antialiasing = SDL_SCALEMODE_LINEAR;
	//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	SDL_srand(0);


	//SDL_Surface *icon = IMG_Load( "icon32.png" );
	//SDL_SetWindowIcon( window, icon );


	SDL_Color bg [] = { {0, 0, 0, 255},
						{85, 85, 85, 255},
						{127, 127, 127, 255},
						{171, 171, 171, 255},
						{255, 255, 255, 255} };
	int selected_color = 1;



	SDL_Texture *TEXTURE = NULL;
	SDL_Surface *SURFACE = NULL;

	IMG_Animation *ANIMATION = NULL;
	int FRAME, NFRAME;
	bool animating = 0;

	int W = 0, H = 0;
	SDL_FRect DST;
	SDL_Rect window_rect = (SDL_Rect){0, 0, width, height};
	SDL_Rect max_window_rect = (SDL_Rect){0, 0, width, height};
	SDL_FRect sel_rect = (SDL_FRect){0,0,0,0};
	int angle_i = 0;
	double ANGLE = 0;
	SDL_FlipMode FLIP = SDL_FLIP_NONE;
	

	float cx = width / 2.0;
	float cy = height / 2.0;
	bool mousePressed = 0;
	int clickX, clickY;

	bool mmpan = 0;// middle mouse pan
	double tx = 0, ty = 0;

	bool fit = 0;

	double zoom_i = 0;
	double zoom = 1;
	bool zoom_in = 0;
	bool zoom_out = 0;
	float zoomV = 0.25;
	//clock_t zoom_t = 0;
	//clock_t zoom_dt = SDL_round(0.1 * CLOCKS_PER_SEC); 

	double panV = 9;
	double panVF = 0.04;//vel factor for the mmpan
	bool pan_up = 0;
	bool pan_down = 0;
	bool pan_left = 0;
	bool pan_right = 0;
	bool rotate_ccw = 0;
	bool rotate_cw = 0;

	bool fullscreen = 0;
	bool minimized = 0;

	
	char *folderpath = NULL;
	bool remote_operation = false;

	str_vec directory_list;

	int INDEX = 0;// of the present file in the list
	


	void load_folderlist( int depth ){

		ok_vec_init( &directory_list );
		if( !SDL_EnumerateDirectory( folderpath, enudir_callback, &directory_list ) ){
			SDL_Log("SDL_EnumerateDirectory (1) error: %s", SDL_GetError());
		}
		depth -= 1;
		while( depth > 0 ){
			int new_subdirs = 0;
			ok_vec_foreach_rev( &directory_list, char *p ) {
				int l = strlen(p);
				if( p[l-1] == '\\' ){
					sprintf( buffer, "%s%s", folderpath, p );
					if( !SDL_EnumerateDirectory( buffer, enudir_callback, &directory_list ) ){
						SDL_Log("SDL_EnumerateDirectory (2) error: %s", SDL_GetError());
					}
					ok_vec_remove( &directory_list, p );
					free( p );
					new_subdirs += 1;
				}
			}
			if( new_subdirs <= 0 ) break;
			depth -= 1;
		}
	}


	int load_image(){

		char path [1024];

		if( remote_operation ){
			sprintf( path, "%s%s", folderpath, ok_vec_get( &directory_list, INDEX ) );
		}else{
			SDL_strlcpy( path, ok_vec_get( &directory_list, INDEX ), bufflen );
		}
		//printf("path: %s\n", path );

		//CP_ACP_to_UTF8( path, buffer ); // CP_ACP_to_UTF8( path );
		//printf("loading path: %s\n", path );

		//sprintf( buffer, "Loading \"%s\"...  [%d / %d]", path, INDEX, ok_vec_count( &directory_list ) );
		SDL_SetWindowTitle( window, buffer );


		int EXT = check_extension( path );

		if( !EXT ) return 0;

		if( TEXTURE != NULL ){
			SDL_DestroyTexture( TEXTURE ); 
			TEXTURE = NULL;
		}
		if( SURFACE != NULL ){
			SDL_DestroySurface( SURFACE ); 
			SURFACE = NULL;
		}
		if( ANIMATION != NULL ){
			IMG_FreeAnimation( ANIMATION );
			ANIMATION = NULL;
		}
		animating = 0;

		int off = 0;
		for (int i = strlen( path ); i >=0 ; --i){
			if( path[i] == '\\' ){
				off = i+1;
				break;
			}
		}

		if( EXT == 4 || EXT == 9 ){//.gif or webp
			ANIMATION = IMG_LoadAnimation( path );

			if( ANIMATION == NULL ){
				printf("bad anim, %s\n", SDL_GetError() );
				goto loadtexture;
			}
			else{
				if( ANIMATION->count == 1 ){
					SURFACE = ANIMATION->frames[0];
					ANIMATION->frames[0]->refcount += 1;
					goto we_surfin;
				}
				else{
					printf( "good anim, %d frames\n", ANIMATION->count );
					W = ANIMATION->w;
					H = ANIMATION->h;
					FRAME = 0;
					NFRAME = clock() + ANIMATION->delays[0];
					animating = 1;
					for (int i = 0; i < ANIMATION->count; ++i ){
						regularize_surface( ANIMATION->frames + i, window_surf );
					}
				}
			}
		}
		else{
			loadtexture:
			TEXTURE = IMG_LoadTexture( renderer, path );
		}
		
		if( animating <= 0 && TEXTURE == NULL ){
			printf("bad texture: %s\n", SDL_GetError());
			
			SURFACE = IMG_Load( path );

			if( SURFACE == NULL ){
				puts( "bad surface" );
				sprintf( buffer, "%s   ERROR: %s", path + off, SDL_GetError() );
				SDL_SetWindowTitle( window, buffer );
				return 0;
			}
			else{
				we_surfin:
				puts("we surfin'");
				regularize_surface( &SURFACE, window_surf );
				W = SURFACE->w;
				H = SURFACE->h;
				if( W < 4000 && H < 4000 ){
					TEXTURE = SDL_CreateTextureFromSurface( renderer, SURFACE );
					SDL_DestroySurface( SURFACE ); 
					SURFACE = NULL;
					antialiasing = 0;
				}
			}
		}
		
		if( TEXTURE != NULL ){
			float fw, fh;
			SDL_GetTextureSize(TEXTURE, &fw, &fh);
			W = fw; H = fh;
			if( antialiasing > 0 && (W <= 256 || H <= 256) ){
				antialiasing = SDL_SCALEMODE_NEAREST;//SDL_SCALEMODE_PIXELART;
				SDL_SetTextureScaleMode( TEXTURE, antialiasing );
			}
		}

		sprintf( buffer, "%s  •  (%d × %d)  •  [%d / %d]", ok_vec_get(&directory_list, INDEX), W, H, 
														   INDEX, ok_vec_count( &directory_list ) );
		SDL_SetWindowTitle( window, buffer );

		DST = (SDL_FRect){0, 0, W, H};
		if( !fit && W < width && H < height ){
			tx = 0.5 * (width  - W);
			ty = 0.5 * (height - H);
			DST.x = tx;
			DST.y = ty;
			zoom_i = 0;
			zoom = 1;
		}
		else{
			fit_rect( &DST, &window_rect );
			tx = DST.x; ty = DST.y;
			zoom = DST.w / (float) W;
			zoom_i = logarithm( 1.1, zoom );
			fit = 1;
		}
		angle_i = 0;
		ANGLE = 0;
		FLIP = SDL_FLIP_NONE;

		return 1;
	}





	if( argc == 2 ){

		//input_path = Win_to_opendir( argv[1] );
		//printf("argv[1]: {%s}\nfor SDL: {%s}\nfor opendir: {%s}\n", argv[1], path_SDL, input_path );
		//printf("argv[1]: %s\n", argv[1] );
		//printf("SDL_GetCurrentDirectory(): %s\n", SDL_GetCurrentDirectory() );


		int len = strlen( argv[1] );
		for (int i = len; i >=0 ; --i){
			if( argv[1][i] == '\\' ){
				folderpath_len = i+1;
				break;
			}
		}
		folderpath = substr( argv[1], 0, folderpath_len );
		//char *name = substr( argv[1], folderpath_len+1, len );
		//printf("folderpath: %s\n", folderpath );

		if( SDL_strncmp( folderpath, SDL_GetCurrentDirectory(), len ) != 0 ){
			remote_operation = true;
		}
		
		load_folderlist( 1 );

		//printf("argv[1]: %s\nSDL_GetCurrentDirectory(): %s\n",argv[1], SDL_GetCurrentDirectory() );

		CP_ACP_to_UTF8( buffer, argv[1] );
		//printf("buffer: %s\n", buffer );
		// find where we are in the directory
		for (int i = 0; i < ok_vec_count( &directory_list ); ++i){
			//printf("[%d]: %s\n", i, ok_vec_get( &directory_list, i ) );
			if( strrcmp( buffer, ok_vec_get( &directory_list, i ) ) == 0 ){
				INDEX = i;
				break;
			}
		}

		load_image();
	}


	SDL_SetRenderDrawColor( renderer, bg[selected_color].r, bg[selected_color].g, bg[selected_color].b, bg[selected_color].a );
	SDL_RenderClear( renderer );
	SDL_RenderPresent(renderer);

	int first = 3;

	//puts("<<Entering Loop>>");
	while ( loop ) { //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> L O O P <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< 

		bool update = first;
		if( first > 0 ) first--;

		while( SDL_PollEvent(&event) ){

			update += 1;

			int psel = INDEX;
			int dir = 0;

			switch (event.type) {
				case SDL_EVENT_QUIT:

					loop = 0;

					break;
				case SDL_EVENT_KEY_DOWN:

						 if( event.key.key == SDLK_LCTRL  || event.key.key == SDLK_RCTRL  ) CTRL  = 1;
					else if( event.key.key == SDLK_LSHIFT || event.key.key == SDLK_RSHIFT ) SHIFT = 1;

					else if( event.key.key == 'h' ) pan_left = 1; 
					else if( event.key.key == 'j' || event.key.key == SDLK_DOWN ) pan_down = 1; 
					else if( event.key.key == 'k' || event.key.key == SDLK_UP   ) pan_up = 1;   
					else if( event.key.key == 'l' ) pan_right = 1;

					else if( event.key.key == 'i' ) zoom_in = 1;
					else if( event.key.key == 'o' ) zoom_out = 1;

					else if( event.key.key == SDLK_KP_PLUS ) zoom_in = 1;
					else if( event.key.key == SDLK_KP_MINUS ) zoom_out = 1;

					else if( event.key.key == SDLK_KP_5 ) zoom_in = 1;
					else if( event.key.key == SDLK_KP_0 ) zoom_out = 1;

					else if( event.key.key == SDLK_KP_2 ) pan_down = 1;
					else if( event.key.key == SDLK_KP_4 ) pan_left = 1; 
					else if( event.key.key == SDLK_KP_6 ) pan_right = 1;
					else if( event.key.key == SDLK_KP_8 ) pan_up = 1;

					break;
				case SDL_EVENT_KEY_UP:;

						 if( event.key.key == SDLK_LCTRL  || event.key.key == SDLK_RCTRL  ) CTRL  = 0;
					else if( event.key.key == SDLK_LSHIFT || event.key.key == SDLK_RSHIFT ) SHIFT = 0;

					else if( event.key.key == SDLK_KP_7 ) rotate_ccw = 1;
					else if( event.key.key == SDLK_KP_9 ) rotate_cw = 1;
					else if( event.key.key == SDLK_KP_DIVIDE ){
						if( FLIP & SDL_FLIP_HORIZONTAL ){
							FLIP &= ~SDL_FLIP_HORIZONTAL;
						}
						else FLIP |= SDL_FLIP_HORIZONTAL;
					}
					else if( event.key.key == SDLK_KP_MULTIPLY ){
						if( FLIP & SDL_FLIP_VERTICAL ){
							FLIP &= ~SDL_FLIP_VERTICAL;
						}
						else FLIP |= SDL_FLIP_VERTICAL;
					}

					else if( event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_1 ){
						INDEX--;
						dir = -1;
					} 
					else if( event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_3 ){
						INDEX++;
						dir = 1;
					}

					else if( event.key.key == 'h' ) pan_left = 0;
					else if( event.key.key == 'j' || event.key.key == SDLK_DOWN ) pan_down = 0;
					else if( event.key.key == 'k' || event.key.key == SDLK_UP   ) pan_up = 0;
					else if( event.key.key == 'l' ) pan_right = 0;

					else if( event.key.key == 'i' ) zoom_in = 0; 
					else if( event.key.key == 'o' ) zoom_out = 0;

					else if( event.key.key == SDLK_KP_PLUS ) zoom_in = 0;
					else if( event.key.key == SDLK_KP_MINUS ) zoom_out = 0;

					else if( event.key.key == SDLK_KP_5 ) zoom_in = 0;
					else if( event.key.key == SDLK_KP_0 ) zoom_out = 0;

					else if( event.key.key == SDLK_KP_2 ) pan_down = 0;
					else if( event.key.key == SDLK_KP_4 ) pan_left = 0; 
					else if( event.key.key == SDLK_KP_6 ) pan_right = 0;
					else if( event.key.key == SDLK_KP_8 ) pan_up = 0;

					else if( event.key.key == ' ' ){
						fit_rect( &DST, &window_rect );
						tx = DST.x; ty = DST.y;
						zoom = DST.w / (float) W;
						zoom_i = logarithm( 1.1, zoom );
						fit = 1;
						angle_i = 0;
						ANGLE = 0;
						FLIP = SDL_FLIP_NONE;
					}
					else if( event.key.key >= '1' && event.key.key <= '9' ){
						zoom = event.key.key - '0';
						zoom_i = logarithm( 1.1, zoom );
						tx = 0.5 * ( width  - (zoom * W) );
						ty = 0.5 * ( height - (zoom * H) );
						DST = (SDL_FRect){ tx,  ty, zoom * W, zoom * H};	
						fit = 0;
						angle_i = 0;
						ANGLE = 0;
						FLIP = SDL_FLIP_NONE;
					}
					else if( event.key.key == 'c' ){
						selected_color--;
						if( selected_color < 0 ) selected_color = 4;
					}
					else if( event.key.key == 'a' ){
						antialiasing++;
						if( antialiasing > 1 ) antialiasing = 0;

						SDL_SetTextureScaleMode( TEXTURE, antialiasing );
						/**SDL_SCALEMODE_NEAREST,  < nearest pixel sampling */
					    /**SDL_SCALEMODE_LINEAR,   < linear filtering */
					    /**SDL_SCALEMODE_PIXELART  < nearest pixel sampling with improved scaling for pixel art */

						//SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, s );
						//SDL_SetHintWithPriority( SDL_HINT_RENDER_SCALE_QUALITY, s, SDL_HINT_OVERRIDE );
					}
					else if( event.key.key == 's' ){
						char pname [512];
						SDL_strlcpy( pname, ok_vec_get( &directory_list, INDEX ), bufflen );

						shuffle_str_list( ok_vec_begin(&directory_list), ok_vec_count(&directory_list) );

						// find where we are in the list
						for (int i = 0; i < ok_vec_count( &directory_list ); ++i){	
							if( strrcmp( pname, ok_vec_get( &directory_list, i ) ) == 0 ){
								INDEX = i;
								break;
							}
						}
					}
					else if( event.key.key == SDLK_F5 ){

						psel = -1;
						char pname [512];
						SDL_strlcpy( pname, ok_vec_get( &directory_list, INDEX ), bufflen );

						destroy_str_vec( &directory_list );
						load_folderlist( 1 );

						// find where we are in the directory
						for (int i = 0; i < ok_vec_count( &directory_list ); ++i){	
							if( strrcmp( pname, ok_vec_get( &directory_list, i ) ) == 0 ){
								INDEX = i;
								break;
							}
						}
					}
					else if( event.key.key == SDLK_F6 ){

						//psel = -1;
						char pname [512];
						SDL_strlcpy( pname, ok_vec_get( &directory_list, INDEX ), bufflen );

						destroy_str_vec( &directory_list );
						load_folderlist( 9999 );

						// find where we are in the list
						for (int i = 0; i < ok_vec_count( &directory_list ); ++i){	
							if( strrcmp( pname, ok_vec_get( &directory_list, i ) ) == 0 ){
								INDEX = i;
								break;
							}
						}

						/*puts("DEEP LIST:\n");
						ok_vec_foreach( &directory_list, char *p ){
							printf("p: %s\n", p );
						}*/
					}
					else if( event.key.key == SDLK_F11 ){
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
					}
					else if( event.key.key == SDLK_ESCAPE ){
						if( fullscreen ){
							SDL_SetWindowFullscreen( window, 0 );
							//SDL_MaximizeWindow( window );
							fullscreen = 0;
						}
					}
					else if( event.key.key == SDLK_DELETE && SHIFT ){
						char path [256];
						SDL_RemovePath( ok_vec_get( &directory_list, INDEX ) );
						//remove_item_from_string_list( &directory_list, INDEX, &list_len );
						ok_vec_remove_at( &directory_list, INDEX );
						SDL_DestroyTexture( TEXTURE );
						TEXTURE = NULL;
						//INDEX++;
						psel--;
						dir = 1;
						/*
							char cmd [259];
							sprintf( cmd, "move %s C:\\$Recycle.Bin", path );
							system( cmd );
						*/
					}

					break;
				case SDL_EVENT_MOUSE_MOTION:

					pmouseX = mouseX;
					pmouseY = mouseY;
					mouseX = event.motion.x;
					mouseY = event.motion.y;
					if( mousePressed ){
						if( CTRL ){
							//int tcx = SDL_round(tx + (zoom * clickX));
							//int tcy = SDL_round(ty + (zoom * clickY));
							sel_rect.x = min( mouseX, clickX );
							sel_rect.y = min( mouseY, clickY );
							sel_rect.w = SDL_abs( mouseX - clickX );
							sel_rect.h = SDL_abs( mouseY - clickY );
						}
						else{
							tx += event.motion.xrel;
							ty += event.motion.yrel;
							DST.x = tx;
							DST.y = ty;
							fit = 0;
						}
					}
					else update -= 1;

					break;
				case SDL_EVENT_MOUSE_BUTTON_DOWN:

					if( CTRL ){
						clickX = mouseX;//SDL_round( (mouseX - tx) / zoom );
						clickY = mouseY;//SDL_round( (mouseY - ty) / zoom );
						sel_rect.x = mouseX;//SDL_round( tx + (zoom * clickX) );
						sel_rect.y = mouseY;//SDL_round( ty + (zoom * clickY) );
						sel_rect.w = 1;
						sel_rect.h = 1;
					}

					if( event.button.button == SDL_BUTTON_MIDDLE ){
						mmpan = 1;
						clickX = mouseX;
						clickY = mouseY;
					}
					else if( event.button.button == SDL_BUTTON_X1 ){
						INDEX--;
						dir = -1;
					} 
					else if( event.button.button == SDL_BUTTON_X2 ){
						INDEX++;
						dir = 1;
					}
					else mousePressed = 1;


					break;
				case SDL_EVENT_MOUSE_BUTTON_UP:

					mousePressed = 0;

					if( CTRL ){
						
						SDL_FRect fitrect = (SDL_FRect){ 0, 0, sel_rect.w, sel_rect.h };
						fit_rect( &fitrect, &window_rect );

						double otx = (sel_rect.x - tx) / zoom;
						double oty = (sel_rect.y - ty) / zoom;
						zoom *= (fitrect.w / (float) sel_rect.w);
						zoom_i = logarithm( 1.1, zoom );
						tx = fitrect.x - (otx * zoom);
						ty = fitrect.y - (oty * zoom);
						DST.x = tx;
						DST.y = ty;
						DST.w = W * zoom;
						DST.h = H * zoom;
						clickX = -1;
					}
					if( event.button.button == SDL_BUTTON_MIDDLE ){
						mmpan = 0;
					}

					break;
				case SDL_EVENT_MOUSE_WHEEL:;

					float xrd = (mouseX - tx) / zoom;
					float yrd = (mouseY - ty) / zoom;
					zoom_i -= event.wheel.y;
					zoom = pow( 1.1, zoom_i );
					tx = mouseX - xrd * zoom;
					ty = mouseY - yrd * zoom;
					DST.x = tx;
					DST.y = ty;
					DST.w = W * zoom;
					DST.h = H * zoom;
					fit = 0;

					break;

				case SDL_EVENT_WINDOW_MINIMIZED:
					minimized = 1;
					break;

				case SDL_EVENT_WINDOW_RESTORED:

					if( !minimized ){
						if( W <= max_window_rect.w && H <= max_window_rect.h ){
							SDL_SetWindowSize( window, W, H );
							zoom_i = 0;
							zoom = 1;
						}
						else{
							DST.w = W;
							DST.h = H;
							fit_rect( &DST, &max_window_rect );
							SDL_SetWindowSize( window, DST.w, DST.h );
							tx = DST.x;
							ty = DST.y;
							zoom = DST.w / (float) W;
							zoom_i = logarithm( 1.1, zoom );
						}
						fit = 1;
						SDL_GetWindowSize( window, &width, &height );
						window_rect.w = width;
						window_rect.h = height;
						cx = width/2;
						cy = height / 2;
					}
					minimized = 0;

					break;

				case SDL_EVENT_WINDOW_RESIZED:
				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				case SDL_EVENT_WINDOW_MAXIMIZED:
				case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
				case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:

					window_surf = SDL_GetWindowSurface( window );
					SDL_GetWindowSize( window, &width, &height );
					window_rect.w = width;
					window_rect.h = height;
					cx = width / 2;
					cy = height / 2;
					if( fit ){
						fit_rect( &DST, &window_rect );
						tx = DST.x; ty = DST.y;
						zoom = DST.w / (float) W;
						zoom_i = logarithm( 1.1, zoom );
					}
				break;

			}

			if( psel != INDEX ){
				int count = 0;

				while( count < ok_vec_count( &directory_list ) ){
					INDEX = cycle( INDEX, 0, ok_vec_count( &directory_list )-1 );
					if( load_image() ){
						break;
					}
					INDEX += dir;
					count++;
				}
			}
		}

		if( pan_up || pan_down || pan_left || pan_right ){

			if( pan_up    ) ty += panV;
			if( pan_down  ) ty -= panV;
			if( pan_left  ) tx += panV;
			if( pan_right ) tx -= panV;
			DST.x = tx;
			DST.y = ty;
			update = 1;
		}
		if( mmpan ){

			tx += panVF * (clickX - mouseX);
			ty += panVF * (clickY - mouseY);
			DST.x = tx;
			DST.y = ty;

			/* THIS WORKS REALLY WELL ACTUALLY!
			   but I eneded up replacing the whole panning system
			if( fabs(trunc(tx)) > 0 ){
				DST.x += trunc(tx);
				tx -= trunc(tx);
			}
			if( fabs(trunc(ty)) > 0 ){
				DST.y += trunc(ty);
				ty -= trunc(ty);
			}*/
			update = 1;
		}
		if( zoom_in || zoom_out ){
			//printf("%d, %d, %d\n", clock(), zoom_t, zoom_dt );
			//if( clock() - zoom_t > zoom_dt ){
			double xrd = (cx - tx) / zoom;
			double yrd = (cy - ty) / zoom;
			if( zoom_in  ) zoom_i += zoomV;
			if( zoom_out ) zoom_i -= zoomV;
			zoom = pow( 1.1, zoom_i );
			tx = cx - xrd * zoom;
			ty = cy - yrd * zoom;
			DST.x = tx;
			DST.y = ty;
			DST.w = W * zoom;
			DST.h = H * zoom;
			fit = 0;
			update = 1;
		}
		if( rotate_cw || rotate_ccw ){
			angle_i += rotate_cw - rotate_ccw;
			ANGLE = 90 * (angle_i % 4);
			rotate_cw = 0;
			rotate_ccw = 0;
			update = 1;
		}

		if( animating ){

			SDL_FillSurfaceRect( window_surf, NULL, SDL_Color_to_Uint32( bg[selected_color]) );
			SDL_Rect copy = (SDL_Rect){ DST.x, DST.y, DST.w, DST.h };
			SDL_BlitSurfaceScaled( ANIMATION->frames[ FRAME ], NULL, window_surf, &copy, antialiasing );
			//	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_BlitSurfaceScaled error: %s", SDL_GetError());
			//}
			clock_t now = clock();
			if( now >= NFRAME ){
				FRAME++;
				if( FRAME >= ANIMATION->count ){
					FRAME = 0;
				}
				NFRAME = now + ANIMATION->delays[ FRAME ];
			}
			SDL_UpdateWindowSurface( window );
			update = 0;
		}

		if( update ){

			if( TEXTURE != NULL ){
				SDL_SetRenderDrawColor( renderer, bg[selected_color].r, bg[selected_color].g, bg[selected_color].b, bg[selected_color].a );
				SDL_RenderClear( renderer );

				if( angle_i != 0 || FLIP != SDL_FLIP_NONE ){
					SDL_RenderTextureRotated( renderer, TEXTURE, NULL, &DST, ANGLE, NULL, FLIP );
				}
				else{
					SDL_RenderTexture( renderer, TEXTURE, NULL, &DST );
				}
			}
			else if( SURFACE != NULL ){

				SDL_FillSurfaceRect( window_surf, NULL, SDL_Color_to_Uint32( bg[selected_color]) );
				SDL_Rect copy = (SDL_Rect){ DST.x, DST.y, DST.w, DST.h };
				SDL_BlitSurfaceScaled( SURFACE, NULL, window_surf, &copy, antialiasing );
			}

			if( mmpan ){
				SDL_SetRenderDrawColor( renderer, 0, 255, 0, 255 );
				SDL_RenderRect( renderer, &(SDL_FRect){clickX-6, clickY-6, 12, 12 } );
				for(float d = 0.25; d < 1; d += 0.25 ){
					float x = clickX + d * (mouseX - clickX);
					float y = clickY + d * (mouseY - clickY);
					SDL_RenderRect( renderer, &(SDL_FRect){ x-3, y-3, 6, 6 } );
				}
			}
			if( mousePressed && CTRL ){
				SDL_SetRenderDrawColor( renderer, 0, 255, 0, 255 );
				//int tcx = DST.x + (zoom * clickX);
				//int tcy = DST.y + (zoom * clickY);
				SDL_RenderRect( renderer, &sel_rect );
			}


			if( TEXTURE != NULL ){
				SDL_RenderPresent(renderer);
			}
			else if( SURFACE != NULL ){
				SDL_UpdateWindowSurface( window );
			}
		}

		SDL_framerateDelay( 16 );

	}//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> / L O O P <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	free( folderpath );
	destroy_str_vec( &directory_list );

	//SDL_DestroySurface( icon );
	SDL_DestroyTexture( TEXTURE );
	SDL_DestroySurface( SURFACE );
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );

	SDL_Quit();

	return 0;
}

