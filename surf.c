#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_image.h>


void regularize_surface( SDL_Surface **S, SDL_Surface *T ){

	if( (*S)->format != T->format ){
		puts("diff formats!! converting...");
		SDL_Surface *temp = SDL_ConvertSurface( *S, T->format );
		SDL_DestroySurface( *S );
		*S = temp;
	}
}

int main(int argc, char const *argv[]){

	SDL_Window *window;
	SDL_Renderer *renderer;

	if( !SDL_Init(SDL_INIT_VIDEO) ) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
		return 3;
	}
	if( !SDL_CreateWindowAndRenderer( "Surf test", 600, 400, 0, &window, &renderer) ){
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError() );
		return 3;
	}

	SDL_SetRenderDrawColor( renderer, 12,12,12,255 );
	SDL_RenderClear( renderer );
	SDL_RenderPresent(renderer);

	SDL_Surface *SURF = IMG_Load( argv[1] );
	SDL_Surface *window_surf = SDL_GetWindowSurface( window );
	regularize_surface( &SURF, window_surf );
	SDL_FillSurfaceRect( window_surf, NULL, 0x00000000 );
	SDL_BlitSurfaceScaled( SURF, NULL, window_surf, &(SDL_Rect){0,0,SURF->w, SURF->h}, 0 );
	SDL_UpdateWindowSurface( window );
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "errors? %s", SDL_GetError() );
	getchar();
	return 0;
}