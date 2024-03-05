#include "basics.h"
#include <SDL.h>
#include <SDL_image.h>

#ifdef WIN32
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif

#include <dirent.h>
#include <errno.h>
#include <locale.h>


void get_filenames( char *directory, char ***list, int *length ){
	setlocale (LC_ALL, "");
    DIR *dir;
    struct dirent *ent;
    dir = opendir( directory );
    *list = NULL;
    *length = 0;
    if (dir != NULL) {
        while ((ent = readdir (dir)) != NULL) {
        	//printf("%s\n", ent->d_name );
            (*length) += 1;
            (*list) = realloc( (*list), (*length) * sizeof(char*) );
            size_t len = strlen( ent->d_name );
            (*list)[ (*length)-1 ] = calloc( len + 2, 1 );
            memcpy( (*list)[ (*length)-1 ], ent->d_name, len );
            if( ent->d_type == DT_DIR ){
            	(*list)[ (*length)-1 ][ len ] = '/';
            }
            (*list)[ (*length)-1 ][ len+1 ] = '\0';
        }
        closedir (dir);
    }
    else { 
        printf("ERROR: could not open directory: %s\n", directory );
    }
}

void remove_item_from_string_list( char ***list, int X, int *len ){
	*len -= 1;
	for (int i = X; i < (*len); ++i){
		(*list)[i] = (*list)[i+1];
	}
	*list = realloc( *list, (*len) * sizeof(char**) );
}

//Case Insensitive String Comparison
int strcicmp(char const *a, char const *b){
    for (;; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if ( d != 0 || !*a || !*b ){
            return d;
        }
    }
}


int check_extension( char *filename ){
	//                           1       2       3       4       5      6        7       8      9
	const char exts [][5] = { ".png", ".jpg", "jpeg", ".gif", ".tif", "tiff", ".ico", ".bmp", "webp" };

	int len = strlen( filename );
	for (int i = 0; i < 9; ++i ){
		if( strcicmp( filename + len -4, exts[i] ) == 0 ){
			return i+1;
		}
	}
	return 0;
}

void regularize_surface( SDL_Surface **S, SDL_Surface *T ){

	if( (*S)->format->format != T->format->format ){
		//puts("diff formats!! converting...");
		SDL_Surface *temp = SDL_ConvertSurface( *S, T->format, 0 );
		SDL_FreeSurface( *S );
		*S = temp;
	}
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~O~~~~~~~~~~| M A I N |~~~~~~~~~~~O~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int main(int argc, char *argv[]){

	//HWND hwnd_win = GetConsoleWindow();
	//ShowWindow(hwnd_win,SW_HIDE);
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Event event;
	int width = 1;
	int height = 1;
	bool loop = 1;

	bool CTRL = 0;
	bool SHIFT = 0;

	int mouseX, mouseY, pmouseX, pmouseY;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
		return 3;
	}
	if (SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED, &window, &renderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
		return 3;
	}
	//SDL_MaximizeWindow( window );
	SDL_GetWindowSize( window, &width, &height );

	SDL_Surface *window_surf = SDL_GetWindowSurface( window );

	int antialiasing = 2;
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");


	IMG_Init( IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_TIF | IMG_INIT_WEBP );


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
	SDL_Rect DST;
	SDL_Rect window_rect = (SDL_Rect){0, 0, width, height};
	SDL_Rect max_window_rect = (SDL_Rect){0, 0, width, height};
	SDL_Rect sel_rect = (SDL_Rect){0,0,0,0};
	int angle_i = 0;
	double ANGLE = 0;
	SDL_RendererFlip FLIP = SDL_FLIP_NONE;
	

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
	//clock_t zoom_dt = lrint(0.1 * CLOCKS_PER_SEC); 

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

	char **directory_list = NULL;
	int list_len = 0;
	int selected_file = 0;
	char *folderpath = NULL;
	int folderpath_len = 0;

	char buffer [512];


	int load_image( char *path ){
		//printf("%d, %s\n", argc, argv[1] );
		printf("path: %s\n", path );

		int EXT = check_extension( path );

		if( !EXT ) return 0;

		if( TEXTURE != NULL ){
			SDL_DestroyTexture( TEXTURE ); 
			TEXTURE = NULL;
		}
		if( SURFACE != NULL ){
			SDL_FreeSurface( SURFACE ); 
			SURFACE = NULL;
		}
		if( ANIMATION != NULL ){
			IMG_FreeAnimation( ANIMATION );
			ANIMATION = NULL;
			animating = 0;
		}

		int off = 0;
		for (int i = strlen( path ); i >=0 ; --i){
			if( path[i] == '\\' ){
				off = i+1;
				break;
			}
		}

		//printf("EXT: %d\n", EXT );
		if( EXT == 4 || EXT == 9 ){//.gif or webp
			ANIMATION = IMG_LoadAnimation( path );

			if( ANIMATION == NULL ){
				//printf("bad anim, %s\n", SDL_GetError() );
				goto reload;
			}
			else{
				//printf("good anim, %d frames\n", ANIMATION->count);
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
		else{
			reload:
			TEXTURE = IMG_LoadTexture( renderer, path );
		}
		
		if( animating <= 0 && TEXTURE == NULL ){
			//printf("bad texture: %s\n", SDL_GetError());
			
			SURFACE = IMG_Load( path );

			if( SURFACE == NULL ){
				//puts( "bad surface");
				sprintf( buffer, "%s   ERROR: %s", path + off, SDL_GetError() );
				SDL_SetWindowTitle( window, buffer );
				return 0;
			}
			else{
				//puts("we surfin'");
				regularize_surface( &SURFACE, window_surf );
				W = SURFACE->w;
				H = SURFACE->h;
				if( W < 4000 && H < 4000 ){
					TEXTURE = SDL_CreateTextureFromSurface( renderer, SURFACE );
					SDL_FreeSurface( SURFACE ); 
					SURFACE = NULL;
					antialiasing = 0;
				}
			}
		}
		
		if( TEXTURE != NULL ){
			SDL_QueryTexture( TEXTURE, NULL, NULL, &W, &H);
			if( antialiasing > 0 && (W <= 256 || H <= 256) ){
				antialiasing = 0;
				SDL_SetHintWithPriority( SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE );
				goto reload;
			}
		}

		sprintf( buffer, "%s   (%d × %d)", path + off, W, H );
		SDL_SetWindowTitle( window, buffer );

		DST = (SDL_Rect){0, 0, W, H};
		if( !fit && W < width && H < height ){
			tx = 0.5 * (width  - W);
			ty = 0.5 * (height - H);
			DST.x = lrint( tx );
			DST.y = lrint( ty );
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


	void load_folderlist( char *path ){

		int len = strlen( path );
		for (int i = len; i >=0 ; --i){
			if( path[i] == '\\' ){
				folderpath_len = i;
				break;
			}
		}
		folderpath = substr( path, 0, folderpath_len );
		char *name = substr( path, folderpath_len+1, len );

		get_filenames( folderpath, &directory_list, &list_len );


		for (int i = 0; i < list_len; ++i){

			int dlen = strlen( directory_list[i] );	

			//hacky converstion to... UTF16? I really don't know, but it makes windows recognize accent marks.
			int8_t *conv = malloc( 2 * dlen );
			int c = 0;
			for (int j = 0; j <= dlen; ++j){
				if( directory_list[i][j] < 0 ){
					conv[c++] = -61;
					conv[c++] = directory_list[i][j] - 64;
				}
				else{
					conv[c++] = directory_list[i][j];
				}
			}
			conv = realloc( conv, c );
			free( directory_list[i] );
			directory_list[i] = conv;


			if( strcmp( name, directory_list[i] ) == 0 ){
				selected_file = i;
				//break;
			}
		}
		free( name );
	}





	if( argc == 2 ){
		
		load_image( argv[1] );
		load_folderlist( argv[1] );
	}	


	//puts("<<Entering Loop>>");
	while ( loop ) { //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> L O O P <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< 

		bool update = 0;

		while( SDL_PollEvent(&event) ){

			update += 1;

			int psel = selected_file;
			int dir = 0;

			switch (event.type) {
				case SDL_QUIT:

					loop = 0;

					break;
				case SDL_KEYDOWN:

					     if( event.key.keysym.sym == SDLK_LCTRL  || event.key.keysym.sym == SDLK_RCTRL  ) CTRL  = 1;
					else if( event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT ) SHIFT = 1;

					else if( event.key.keysym.sym == 'h' ) pan_left = 1; 
					else if( event.key.keysym.sym == 'j' || event.key.keysym.sym == SDLK_DOWN ) pan_down = 1; 
					else if( event.key.keysym.sym == 'k' || event.key.keysym.sym == SDLK_UP   ) pan_up = 1;   
					else if( event.key.keysym.sym == 'l' ) pan_right = 1;

					else if( event.key.keysym.sym == 'i' ) zoom_in = 1;
					else if( event.key.keysym.sym == 'o' ) zoom_out = 1;

					else if( event.key.keysym.sym == SDLK_KP_PLUS ) zoom_in = 1;
					else if( event.key.keysym.sym == SDLK_KP_MINUS ) zoom_out = 1;

					else if( event.key.keysym.sym == SDLK_KP_5 ) zoom_in = 1;
					else if( event.key.keysym.sym == SDLK_KP_0 ) zoom_out = 1;

					else if( event.key.keysym.sym == SDLK_KP_2 ) pan_down = 1;
					else if( event.key.keysym.sym == SDLK_KP_4 ) pan_left = 1; 
					else if( event.key.keysym.sym == SDLK_KP_6 ) pan_right = 1;
					else if( event.key.keysym.sym == SDLK_KP_8 ) pan_up = 1;

					break;
				case SDL_KEYUP:;

					     if( event.key.keysym.sym == SDLK_LCTRL  || event.key.keysym.sym == SDLK_RCTRL  ) CTRL  = 0;
					else if( event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT ) SHIFT = 0;

					else if( event.key.keysym.sym == SDLK_KP_7 ) rotate_ccw = 1;
					else if( event.key.keysym.sym == SDLK_KP_9 ) rotate_cw = 1;
					else if( event.key.keysym.sym == SDLK_KP_DIVIDE ){
						if( FLIP & SDL_FLIP_HORIZONTAL ){
							FLIP &= ~SDL_FLIP_HORIZONTAL;
						}
						else FLIP |= SDL_FLIP_HORIZONTAL;
					}
					else if( event.key.keysym.sym == SDLK_KP_MULTIPLY ){
						if( FLIP & SDL_FLIP_VERTICAL ){
							FLIP &= ~SDL_FLIP_VERTICAL;
						}
						else FLIP |= SDL_FLIP_VERTICAL;
					}

					else if( event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_KP_1 ){
						selected_file--;
						dir = -1;
					} 
					else if( event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_KP_3 ){
						selected_file++;
						dir = 1;
					}

					else if( event.key.keysym.sym == 'h' ) pan_left = 0;
					else if( event.key.keysym.sym == 'j' || event.key.keysym.sym == SDLK_DOWN ) pan_down = 0;
					else if( event.key.keysym.sym == 'k' || event.key.keysym.sym == SDLK_UP   ) pan_up = 0;
					else if( event.key.keysym.sym == 'l' ) pan_right = 0;

					else if( event.key.keysym.sym == 'i' ) zoom_in = 0; 
					else if( event.key.keysym.sym == 'o' ) zoom_out = 0;

					else if( event.key.keysym.sym == SDLK_KP_PLUS ) zoom_in = 0;
					else if( event.key.keysym.sym == SDLK_KP_MINUS ) zoom_out = 0;

					else if( event.key.keysym.sym == SDLK_KP_5 ) zoom_in = 0;
					else if( event.key.keysym.sym == SDLK_KP_0 ) zoom_out = 0;

					else if( event.key.keysym.sym == SDLK_KP_2 ) pan_down = 0;
					else if( event.key.keysym.sym == SDLK_KP_4 ) pan_left = 0; 
					else if( event.key.keysym.sym == SDLK_KP_6 ) pan_right = 0;
					else if( event.key.keysym.sym == SDLK_KP_8 ) pan_up = 0;

					else if( event.key.keysym.sym == ' ' ){
						fit_rect( &DST, &window_rect );
						tx = DST.x; ty = DST.y;
						zoom = DST.w / (float) W;
						zoom_i = logarithm( 1.1, zoom );
						fit = 1;
						angle_i = 0;
						ANGLE = 0;
						FLIP = SDL_FLIP_NONE;
					}
					else if( event.key.keysym.sym >= '1' && event.key.keysym.sym <= '9' ){
						zoom = event.key.keysym.sym - '0';
						zoom_i = logarithm( 1.1, zoom );
						tx = 0.5 * ( width  - (zoom * W) );
						ty = 0.5 * ( height - (zoom * H) );
						DST = (SDL_Rect){lrint(tx), lrint(ty), zoom * W, zoom * H};	
						fit = 0;
						angle_i = 0;
						ANGLE = 0;
						FLIP = SDL_FLIP_NONE;
					}
					else if( event.key.keysym.sym == 'c' ){
						selected_color--;
						if( selected_color < 0 ) selected_color = 4;
					}
					else if( event.key.keysym.sym == 'a' ){
						//puts("hi");
						antialiasing++;
						if( antialiasing > 2 ) antialiasing = 0;
						char s [2];
						sprintf( s, "%d", antialiasing );
						printf("%s, %d\n", s, antialiasing );
						//SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, s );
						SDL_SetHintWithPriority( SDL_HINT_RENDER_SCALE_QUALITY, s, SDL_HINT_OVERRIDE );

						//RELOAD
						char *path [256];
						sprintf( path, "%s\\%s", folderpath, directory_list[ selected_file ] );
						//printf("%d %s %s\n\n", selected_file, folderpath, path );
						SDL_DestroyTexture( TEXTURE );
						TEXTURE = IMG_LoadTexture( renderer, path );

					}
					else if( event.key.keysym.sym == SDLK_F5 ){

						psel = -1;

						sprintf( buffer, "%s\\%s", folderpath, directory_list[ selected_file ] );

						for (int i = 0; i < list_len; ++i){
							free( directory_list[i] );
						}
						free( directory_list );
						free( folderpath );
						
						load_folderlist( buffer );

					}
					else if( event.key.keysym.sym == SDLK_F11 ){
						if( fullscreen ){
							SDL_SetWindowFullscreen( window, 0 );
							//
							fullscreen = 0;
						}
						else{
							SDL_SetWindowFullscreen( window, SDL_WINDOW_FULLSCREEN_DESKTOP );
							fullscreen = 1;
						}
						SDL_GetWindowSize( window, &width, &height );
						window_rect.w = width;
						window_rect.h = height;
						cx = width/2;
						cy = height / 2;
					}
					else if( event.key.keysym.sym == SDLK_ESCAPE ){
						if( fullscreen ){
							SDL_SetWindowFullscreen( window, 0 );
							//SDL_MaximizeWindow( window );
							fullscreen = 0;
						}
					}
					else if( event.key.keysym.sym == SDLK_DELETE && SHIFT ){
						char *path [256];
						sprintf( path, "%s\\%s", folderpath, directory_list[ selected_file ] );
						//printf("%s\n", path );
						remove( path );
						remove_item_from_string_list( &directory_list, selected_file, &list_len );
						SDL_DestroyTexture( TEXTURE );
						TEXTURE = NULL;
						//selected_file++;
						psel--;
						dir = 1;
						/*
							char cmd [259];
							sprintf( cmd, "move %s C:\\$Recycle.Bin", path );
							system( cmd );
						*/
					}

					break;
				case SDL_MOUSEMOTION:

					pmouseX = mouseX;
					pmouseY = mouseY;
					mouseX = event.motion.x;
					mouseY = event.motion.y;
					if( mousePressed ){
						if( CTRL ){
							//int tcx = lrint(tx + (zoom * clickX));
							//int tcy = lrint(ty + (zoom * clickY));
							sel_rect.x = min( mouseX, clickX );
							sel_rect.y = min( mouseY, clickY );
							sel_rect.w = abs( mouseX - clickX );
							sel_rect.h = abs( mouseY - clickY );
						}
						else{
							tx += event.motion.xrel;
							ty += event.motion.yrel;
							DST.x = lrint( tx );
							DST.y = lrint( ty );
							fit = 0;
						}
					}
					else update -= 1;

					break;
				case SDL_MOUSEBUTTONDOWN:

					if( CTRL ){
						clickX = mouseX;//lrint( (mouseX - tx) / zoom );
						clickY = mouseY;//lrint( (mouseY - ty) / zoom );
						sel_rect.x = mouseX;//lrint( tx + (zoom * clickX) );
						sel_rect.y = mouseY;//lrint( ty + (zoom * clickY) );
						sel_rect.w = 1;
						sel_rect.h = 1;
					}

					if( event.button.button == SDL_BUTTON_MIDDLE ){
						mmpan = 1;
						clickX = mouseX;
						clickY = mouseY;
					}
					else if( event.button.button == SDL_BUTTON_X1 ){
						selected_file--;
						dir = -1;
					} 
					else if( event.button.button == SDL_BUTTON_X2 ){
						selected_file++;
						dir = 1;
					}
					else mousePressed = 1;


					break;
				case SDL_MOUSEBUTTONUP:

					mousePressed = 0;

					if( CTRL ){
						
						SDL_Rect fitrect = (SDL_Rect){ 0, 0, sel_rect.w, sel_rect.h };
						fit_rect( &fitrect, &window_rect );

						double otx = (sel_rect.x - tx) / zoom;
						double oty = (sel_rect.y - ty) / zoom;
						zoom *= (fitrect.w / (float) sel_rect.w);
						zoom_i = logarithm( 1.1, zoom );
						tx = fitrect.x - (otx * zoom);
						ty = fitrect.y - (oty * zoom);
						DST.x = lrint( tx );
						DST.y = lrint( ty );
						DST.w = W * zoom;
						DST.h = H * zoom;
						clickX = -1;
					}
					if( event.button.button == SDL_BUTTON_MIDDLE ){
						mmpan = 0;
					}

					break;
				case SDL_MOUSEWHEEL:;

					float xrd = (mouseX - tx) / zoom;
					float yrd = (mouseY - ty) / zoom;
					zoom_i -= event.wheel.y;
					zoom = pow( 1.1, zoom_i );
					tx = mouseX - xrd * zoom;
					ty = mouseY - yrd * zoom;
					DST.x = lrint( tx );
					DST.y = lrint( ty );
					DST.w = W * zoom;
					DST.h = H * zoom;
					fit = 0;

					break;
				case SDL_WINDOWEVENT:

					switch( event.window.event ){

						case SDL_WINDOWEVENT_MINIMIZED:
							minimized = 1;
							break;

						case SDL_WINDOWEVENT_RESTORED:

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

						case SDL_WINDOWEVENT_SIZE_CHANGED:
						case SDL_WINDOWEVENT_RESIZED     :
						case SDL_WINDOWEVENT_MAXIMIZED   :
							
							window_surf = SDL_GetWindowSurface( window );
							SDL_GetWindowSize( window, &width, &height );
							window_rect.w = width;
							window_rect.h = height;
							cx = width/2;
							cy = height / 2;
							if( fit ){
								fit_rect( &DST, &window_rect );
								tx = DST.x; ty = DST.y;
								zoom = DST.w / (float) W;
								zoom_i = logarithm( 1.1, zoom );
							}
						break;
					}

					break;
			}

			if( psel != selected_file ){
				int count = 0;

				while( count < list_len ){
					selected_file = cycle( selected_file, 0, list_len-1 );
					sprintf( buffer, "%s\\%s", folderpath, directory_list[ selected_file ] );//printf("%d %s %s\n\n", selected_file, folderpath, path );
					if( load_image( buffer ) ){
						break;
					}
					selected_file += dir;
					count++;
				}
			}
		}

		if( pan_up || pan_down || pan_left || pan_right ){

			if( pan_up    ) ty += panV;
			if( pan_down  ) ty -= panV;
			if( pan_left  ) tx += panV;
			if( pan_right ) tx -= panV;
			DST.x = lrint( tx );
			DST.y = lrint( ty );
			update = 1;
		}
		if( mmpan ){

			tx += panVF * (clickX - mouseX);
			ty += panVF * (clickY - mouseY);
			DST.x = lrint( tx );
			DST.y = lrint( ty );

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
			DST.x = lrint( tx );
			DST.y = lrint( ty );
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

			SDL_FillRect( window_surf, NULL, SDL_Color_to_Uint32( bg[selected_color]) );
			SDL_Rect copy = DST;
			SDL_BlitScaled( ANIMATION->frames[ FRAME ], NULL, window_surf, &copy );
			//	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_BlitScaled error: %s", SDL_GetError());
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
					SDL_RenderCopyEx( renderer, TEXTURE, NULL, &DST, ANGLE, NULL, FLIP );
				}
				else{
					SDL_RenderCopy( renderer, TEXTURE, NULL, &DST );
				}
			}
			else if( SURFACE != NULL ){

				SDL_FillRect( window_surf, NULL, SDL_Color_to_Uint32( bg[selected_color]) );
				SDL_Rect copy = DST;
				SDL_BlitScaled( SURFACE, NULL, window_surf, &copy );
			}

			if( mmpan ){
				SDL_SetRenderDrawColor( renderer, 0, 255, 0, 255 );
				SDL_RenderDrawRect( renderer, &(SDL_Rect){clickX-6, clickY-6, 12, 12 } );
				for(float d = 0.25; d < 1; d += 0.25 ){
					float x = clickX + d * (mouseX - clickX);
					float y = clickY + d * (mouseY - clickY);
					SDL_RenderDrawRect( renderer, &(SDL_Rect){ x-3, y-3, 6, 6 } );
				}
			}
			if( mousePressed && CTRL ){
				SDL_SetRenderDrawColor( renderer, 0, 255, 0, 255 );
				int tcx = DST.x + (zoom * clickX);
				int tcy = DST.y + (zoom * clickY);
				SDL_RenderDrawRect( renderer, &sel_rect );
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

	IMG_Quit();
	for (int i = 0; i < list_len; ++i){
		free( directory_list[i] );
	}
	free( directory_list );
	free( folderpath );
	//SDL_FreeSurface( icon );
	SDL_DestroyTexture( TEXTURE );
	SDL_FreeSurface( SURFACE );
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );

	SDL_Quit();

	return 0;
}

