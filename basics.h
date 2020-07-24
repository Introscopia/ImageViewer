#ifndef BASICS_H_INCLUDED
#define BASICS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include <SDL.h>


#define PHI         (double) 1.618033988749894848204586834365638117720309179805762862135
#define TWO_PI      (double) 6.283185307179586476925286766559005768394338798750211641949
#define PI          (double) 3.141592653589793238462643383279502884197169399375105820974
#define HALF_PI     (double) 1.570796326794896619231321691639751442098584699687552910487
#define THIRD_PI    (double) 1.047197551196597746154214461093167628065723133125035273658
#define QUARTER_PI  (double) 0.785398163397448309615660845819875721049292349843776455243
#define FIFTH_PI    (double) 0.628318530717958647692528676655900576839433879875021164194
#define SIXTH_PI    (double) 0.523598775598298873077107230546583814032861566562517636829
#define EIGTH_PI    (double) 0.392699081698724154807830422909937860524646174921888227621
#define TWELFTH_PI  (double) 0.261799387799149436538553615273291907016430783281258818414
#define ONE_OVER_PI (double) 0.318309886183790671537767526745028724068919291480912897495

//const SDL_Color black = {0, 0, 0, 255};
//const SDL_Color white = {255, 255, 255, 255};

Uint32 rmask, gmask, bmask, amask;

typedef char bool;


//#define remove_element( ptr, len, ind, siz ) for( int H = ind; H < *len; ++H )




double sq( double a );

double logarithm( double base, double x );

/* RANDOM: "array rules" min inclusive, max not inclusive
	int co  [7];
    for (int i = 0; i < 7; ++i) co[i] = 0;
    for (int i = 0; i < 1000; ++i){
       int x = random(1, 8);
       co[x-1]++;
    }
    for (int i = 0; i < 7; ++i) printf("%d: %d, ", i+1, co[i] );
    puts(".");
*/
int random( int min, int max );
double random_angle();

double lerp(double start, double stop, double amt);

double           map(double value, double source_lo, double source_hi, double dest_lo, double dest_hi);
double ellipticalMap(double value, double source_lo, double source_hi, double dest_lo, double dest_hi);
double    sigmoidMap(double value, double source_lo, double source_hi, double dest_lo, double dest_hi);
double advSigmoidMap(double value, double source_lo, double source_hi, double Slo, double Shi, double dest_lo, double dest_hi);

int constrain( int a, int min, int max );
float constrainf( float a, float min, float max );
double constrainD( double a, double min, double max );

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

double degrees( double radians );
double radians( double degrees );

double rectify_angle( double a );

// String split
void strspl( char *string, const char *delimiters, char ***list, int *size );
// String Count character
int strcchr( char *string, char C );
// sub-string. allocates a new char*. start inclusive, stop not-inclusivve.
char *substr( char *string, int start, int stop );
// insert char at position. returns whether it fit into the size or not
bool str_insert_char( char *string, char C, int pos, int size );
void str_delete_char( char *string, int pos, int len );

char *strtrim( char *string );
void strtrim_trailingNL( char *string );

bool fseek_lines( FILE* f, int N );
bool fseek_string( FILE *f, char *str );

Uint16 *ascii_to_unicode( char *str );

bool cursor_in_rect( SDL_Event *event, SDL_Rect *R );
bool coordinates_in_Rect( float x, float y, SDL_Rect *R );
bool coordinates_in_FRect( float x, float y, SDL_FRect *R );
bool rect_overlap( SDL_FRect *A, SDL_FRect *B );
bool intersecting_or_touching( SDL_Rect *A, SDL_Rect *B);
SDL_Rect add_rects( SDL_Rect *A, SDL_Rect *B);
//scale and translate A to fit inside B, centralized
void fit_rect( SDL_Rect *A, SDL_Rect *B );

bool alphanumeric( char c );
bool lower_case( char c );
bool upper_case( char c );
bool numeral( char c );
bool signed_numeral( char c );
bool punctuation_or_symbol( char c );
bool ascii_text( char c );
char shifted_keys( char c );

typedef struct {
    int i, j;
} index2d;

#endif