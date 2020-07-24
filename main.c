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
        printf("ERROR: could not open vessel directory: %s\n", directory );
    }
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~O~~~~~~~~~~| M A I N |~~~~~~~~~~~O~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int main(int argc, char *argv[]){

	//HWND hwnd_win = GetConsoleWindow();
	//ShowWindow(hwnd_win,SW_HIDE);
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Event event;
	int width = 640;
	int height = 480;
	//int cx, cy;
	bool loop = 1;

	int mouseX, mouseY, pmouseX, pmouseY;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
		return 3;
	}
	if (SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
		return 3;
	}
	SDL_MaximizeWindow( window );
	SDL_GetWindowSize( window, &width, &height );
	//cx = width / 2;
	//cy = height / 2;

	IMG_Init( IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_TIF | IMG_INIT_WEBP );


	SDL_Surface *icon = IMG_Load( "icon32.png" );
	SDL_SetWindowIcon( window, icon );


	SDL_Color bg [4] = { {0, 0, 0, 255},
						 {55, 55, 55, 255},
						 {200, 200, 200, 255},
	                     {255, 255, 255, 255} };
	int selected_color = 2;
	//Uint32 rmask, gmask, bmask, amask;
	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
	#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
	#endif


	SDL_Texture *TEXTURE = NULL;
	int W = 0, H = 0;
	SDL_Rect DST;
	SDL_Rect window_rect = (SDL_Rect){0, 0, width, height};
	bool dragging;
	bool fit = 0;
	int zoomI = 0;
	double zoom = 1;
	double tx = 0, ty = 0;

	char **directory_list = NULL;
	int list_len = 0;
	int selected_file = 0;
	char *folderpath = NULL;
	int folderpath_len = 0;

	if( argc == 2 ){
		//printf("%d, %s\n", argc, argv[1] );
		TEXTURE = IMG_LoadTexture( renderer, argv[1] );
		
		SDL_QueryTexture( TEXTURE, NULL, NULL, &W, &H);

		DST = (SDL_Rect){0, 0, W, H};
		if( W < width && H < height ){
			DST.x = lrint( 0.5 * (width  - W) );
			DST.y = lrint( 0.5 * (height - H) );
		}
		else{
			fit_rect( &DST, &window_rect );
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
		SDL_SetWindowTitle( window, name );

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

			update = 1;

			switch (event.type) {
				case SDL_QUIT:

					loop = 0;

					break;
				case SDL_KEYDOWN:
						
					break;
				case SDL_KEYUP:;
					int psel = selected_file;
					int dir = 0;
					if( event.key.keysym.sym == SDLK_LEFT ){
						selected_file--;
						dir = -1;
					} 
					else if( event.key.keysym.sym == SDLK_RIGHT ){
						selected_file++;
						dir = 1;
					}
					else if( event.key.keysym.sym == ' ' ){
						fit_rect( &DST, &window_rect );
						zoom = DST.w / (float) W;
						zoomI = logarithm( 1.1, zoom );
						fit = 1;
					}
					else if( event.key.keysym.sym == '1' ){
						DST = (SDL_Rect){lrint( 0.5 * (width  - W) ), lrint( 0.5 * (height - H) ), W, H};
						zoom = 1;
						zoomI = 0;
					}
					else if( event.key.keysym.sym == 'c' ){
						selected_color++;
						if( selected_color >= 4 ) selected_color = 0;
					}



					if( psel != selected_file ){

						bool is_image = 0;
						int count = 0;
						while( !is_image ){
							if( selected_file < 0 ) selected_file = list_len-1;
							if( selected_file >= list_len ) selected_file = 0;

							int len = strlen( directory_list[ selected_file ] );
							char *ext = substr( directory_list[ selected_file ], len-4, len );
							if( strcmp( ext, ".png") == 0 ||
							    strcmp( ext, ".jpg") == 0 || 
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
							SDL_SetWindowTitle( window, directory_list[ selected_file ] );
							char *path [256];
							sprintf( path, "%s\\%s", folderpath, directory_list[ selected_file ] );
							//printf("%d %s %s\n\n", selected_file, folderpath, path );
							SDL_DestroyTexture( TEXTURE );
							TEXTURE = IMG_LoadTexture( renderer, path );
		
							SDL_QueryTexture( TEXTURE, NULL, NULL, &W, &H);

							DST = (SDL_Rect){0, 0, W, H};
							window_rect = (SDL_Rect){0, 0, width, height};
							if( W < width && H < height ){
								DST.x = lrint( 0.5 * (width  - W) );
								DST.y = lrint( 0.5 * (height - H) );
								zoomI = 0;
								zoom = 1;
							}
							else{
								fit_rect( &DST, &window_rect );
								zoom = DST.w / (float) W;
								zoomI = logarithm( 1.1, zoom );
								fit = 1;
							}
						}
					}

					break;
				case SDL_MOUSEMOTION:

					pmouseX = mouseX;
					pmouseY = mouseY;
					mouseX = event.motion.x;
					mouseY = event.motion.y;
					if( dragging ){
						DST.x += event.motion.xrel;
						DST.y += event.motion.yrel;
						fit = 0;
					}

					break;
				case SDL_MOUSEBUTTONDOWN:

					dragging = 1;

					break;
				case SDL_MOUSEBUTTONUP:

					dragging = 0;

					break;
				case SDL_MOUSEWHEEL:;

					float xrd = (mouseX - DST.x) / zoom;
					float yrd = (mouseY - DST.y) / zoom;
					zoomI -= event.wheel.y;
					zoom = pow( 1.1, zoomI );
					DST.x = mouseX - xrd * zoom;
					DST.y = mouseY - yrd * zoom;
					DST.w = W * zoom;
					DST.h = H * zoom;
					fit = 0;

					break;
				case SDL_WINDOWEVENT:

					if( event.window.event == SDL_WINDOWEVENT_RESIZED      ||
						event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
						event.window.event == SDL_WINDOWEVENT_MAXIMIZED    ||
						event.window.event == SDL_WINDOWEVENT_RESTORED     ){

						SDL_GetWindowSize( window, &width, &height );
						window_rect.w = width;
						window_rect.h = height;
						if( fit ){
							fit_rect( &DST, &window_rect );
							zoom = DST.w / (float) W;
							zoomI = logarithm( 1.1, zoom );
						}
					}

					break;
			}
		}

		if( update ){
			SDL_SetRenderDrawColor( renderer, bg[selected_color].r, bg[selected_color].g, bg[selected_color].b, bg[selected_color].a );
			SDL_RenderClear( renderer );

			SDL_RenderCopy( renderer, TEXTURE, NULL, &DST );

			SDL_RenderPresent(renderer);
		}

	}//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> / L O O P <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	IMG_Quit();
	for (int i = 0; i < list_len; ++i){
		free( directory_list[i] );
	}
	free( directory_list );
	free( folderpath );
	SDL_FreeSurface( icon );
	SDL_DestroyTexture( TEXTURE );
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow( window );

	SDL_Quit();

	return 0;
}

