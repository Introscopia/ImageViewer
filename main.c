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
	int W = 0, H = 0;
	SDL_Rect DST;
	SDL_Rect window_rect = (SDL_Rect){0, 0, width, height};
	SDL_Rect max_window_rect = (SDL_Rect){0, 0, width, height};
	SDL_Rect sel_rect = (SDL_Rect){0,0,0,0};

	float cx = width / 2.0;
	float cy = height / 2.0;
	bool mousePressed;
	int clickX, clickY;

	bool mmpan = 0;// middle mouse pan
	double tx = 0, ty = 0;

	bool fit = 0;

	double zoomI = 0;
	double zoom = 1;
	bool zoom_in = 0;
	bool zoom_out = 0;
	//clock_t zoom_t = 0;
	//clock_t zoom_dt = lrint(0.1 * CLOCKS_PER_SEC); 

	double panV = 3;
	bool pan_up = 0;
	bool pan_down = 0;
	bool pan_left = 0;
	bool pan_right = 0;

	bool fullscreen = 0;
	bool minimized = 0;

	char **directory_list = NULL;
	int list_len = 0;
	int selected_file = 0;
	char *folderpath = NULL;
	int folderpath_len = 0;

	char buffer [512];

	if( argc == 2 ){
		//printf("%d, %s\n", argc, argv[1] );
		TEXTURE = IMG_LoadTexture( renderer, argv[1] );
		
		SDL_QueryTexture( TEXTURE, NULL, NULL, &W, &H);

		DST = (SDL_Rect){0, 0, W, H};
		if( W < width && H < height ){
			tx = 0.5 * (width  - W);
			ty = 0.5 * (height - H);
			DST.x = lrint( tx );
			DST.y = lrint( ty );
		}
		else{
			fit_rect( &DST, &window_rect );
			tx = DST.x; ty = DST.y;
			zoom = DST.w / (float) W;
			zoomI = logarithm( 1.1, zoom );
			fit = 1;
		}

		int len = strlen( argv[1] );
		for (int i = len; i >=0 ; --i){
			if( argv[1][i] == '\\' ){
				folderpath_len = i;
				break;
			}
		}
		folderpath = substr( argv[1], 0, folderpath_len );
		char *name = substr( argv[1], folderpath_len+1, len );

		if( TEXTURE == NULL ){
			sprintf( buffer, "%s   ERROR: %s", name, SDL_GetError() );
		}
		else{
			sprintf( buffer, "%s   (%d × %d)", name, W, H );
		}
		SDL_SetWindowTitle( window, buffer );
		

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

					break;
				case SDL_KEYUP:;

					     if( event.key.keysym.sym == SDLK_LCTRL  || event.key.keysym.sym == SDLK_RCTRL  ) CTRL  = 0;
					else if( event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT ) SHIFT = 0;

					else if( event.key.keysym.sym == SDLK_LEFT ){
						selected_file--;
						dir = -1;
					} 
					else if( event.key.keysym.sym == SDLK_RIGHT ){
						selected_file++;
						dir = 1;
					}

					else if( event.key.keysym.sym == 'h' ) pan_left = 0;
					else if( event.key.keysym.sym == 'j' || event.key.keysym.sym == SDLK_DOWN ) pan_down = 0;
					else if( event.key.keysym.sym == 'k' || event.key.keysym.sym == SDLK_UP   ) pan_up = 0;
					else if( event.key.keysym.sym == 'l' ) pan_right = 0;

					else if( event.key.keysym.sym == 'i' ) zoom_in = 0; 
					else if( event.key.keysym.sym == 'o' ) zoom_out = 0;

					else if( event.key.keysym.sym == ' ' ){
						fit_rect( &DST, &window_rect );
						tx = DST.x; ty = DST.y;
						zoom = DST.w / (float) W;
						zoomI = logarithm( 1.1, zoom );
						fit = 1;
					}
					else if( event.key.keysym.sym == '1' ){
						tx = 0.5 * (width  - W);
						ty = 0.5 * (height - H);
						DST = (SDL_Rect){lrint(tx), lrint(ty), W, H};
						zoom = 1;
						zoomI = 0;
					}
					else if( event.key.keysym.sym == 'c' ){
						selected_color++;
						if( selected_color >= 5 ) selected_color = 0;
					}
					else if( event.key.keysym.sym == 'a' ){
						puts("hi");
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
						zoomI = logarithm( 1.1, zoom );
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
					zoomI -= event.wheel.y;
					zoom = pow( 1.1, zoomI );
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
									zoomI = 0;
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
									zoomI = logarithm( 1.1, zoom );
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
						
							SDL_GetWindowSize( window, &width, &height );
							window_rect.w = width;
							window_rect.h = height;
							cx = width/2;
							cy = height / 2;
							if( fit ){
								fit_rect( &DST, &window_rect );
								tx = DST.x; ty = DST.y;
								zoom = DST.w / (float) W;
								zoomI = logarithm( 1.1, zoom );
							}
						break;
					}

					break;
			}

			if( psel != selected_file ){
				bool is_image = 0;
				int count = 0;
				while( !is_image ){
					if( selected_file < 0 ){
						if( list_len <= 1 ) break;
						selected_file = list_len-1;
					}
					if( selected_file >= list_len ){
						if( list_len <= 1 ) break;
						selected_file = 0;
					}


					int len = strlen( directory_list[ selected_file ] );
					char *ext = substr( directory_list[ selected_file ], len-4, len );
					if( strcmp( ext, ".png") == 0 ||
						strcmp( ext, ".PNG") == 0 ||
					    strcmp( ext, ".jpg") == 0 || 
					    strcmp( ext, ".JPG") == 0 || 
					    strcmp( ext, "jpeg") == 0 ||
					    strcmp( ext, ".gif") == 0 ||
					    strcmp( ext, ".tif") == 0 ||
					    strcmp( ext, "tiff") == 0 ||
					    strcmp( ext, ".ico") == 0 ||
					    strcmp( ext, ".bmp") == 0 ||
					    strcmp( ext, "webp") == 0 ){

						is_image = 1;
					}
					else{
						selected_file += dir;
						count++;
						if( count >= list_len ) break;
					}
					free( ext );
				}

				if( is_image ){

					char *path [256];
					sprintf( path, "%s\\%s", folderpath, directory_list[ selected_file ] );
					//printf("%d %s %s\n\n", selected_file, folderpath, path );
					SDL_DestroyTexture( TEXTURE );
					TEXTURE = IMG_LoadTexture( renderer, path );

					SDL_QueryTexture( TEXTURE, NULL, NULL, &W, &H);

					DST = (SDL_Rect){0, 0, W, H};
					window_rect = (SDL_Rect){0, 0, width, height};
					if( W < width && H < height ){
						tx = 0.5 * (width  - W);
						ty = 0.5 * (height - H);
						DST.x = lrint( tx );
						DST.y = lrint( ty );
						zoomI = 0;
						zoom = 1;
					}
					else{
						fit_rect( &DST, &window_rect );
						tx = DST.x; ty = DST.y;
						zoom = DST.w / (float) W;
						zoomI = logarithm( 1.1, zoom );
						fit = 1;
					}

					if( TEXTURE == NULL ){
						sprintf( buffer, "%s   ERROR: %s", directory_list[ selected_file ], SDL_GetError() );
					}
					else{
						sprintf( buffer, "%s   (%d × %d)", directory_list[ selected_file ], W, H );
					}
					SDL_SetWindowTitle( window, buffer );
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

			tx += 0.01 * (clickX - mouseX);
			ty += 0.01 * (clickY - mouseY);
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
			if( zoom_in  ) zoomI += 0.075;
			if( zoom_out ) zoomI -= 0.075;
			zoom = pow( 1.1, zoomI );
			tx = cx - xrd * zoom;
			ty = cy - yrd * zoom;
			DST.x = lrint( tx );
			DST.y = lrint( ty );
			DST.w = W * zoom;
			DST.h = H * zoom;
			fit = 0;
			update = 1;
			//	zoom_t = clock();
			//}
		}

		if( update ){

			SDL_SetRenderDrawColor( renderer, bg[selected_color].r, bg[selected_color].g, bg[selected_color].b, bg[selected_color].a );
			SDL_RenderClear( renderer );

			SDL_RenderCopy( renderer, TEXTURE, NULL, &DST );

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

			SDL_RenderPresent(renderer);
		}

	}//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> / L O O P <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	IMG_Quit();
	for (int i = 0; i < list_len; ++i){
		free( directory_list[i] );
	}
	free( directory_list );
	free( folderpath );
	//SDL_FreeSurface( icon );
	SDL_DestroyTexture( TEXTURE );
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );

	SDL_Quit();

	return 0;
}

