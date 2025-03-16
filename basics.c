#include "basics.h"


Uint32 SDL_Color_to_Uint32( SDL_Color C ){
	#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		return C.a << 24 | C.b << 16 | C.g << 8 | C.r;
	#else
		return C.r << 24 | C.g << 16 | C.b << 8 | C.a;
	#endif
}

double sq( double a ){
	return a * a;
}

double logarithm( double base, double x ){
	return SDL_log10( x ) / SDL_log10( base );
}
/*
int random( int min, int max ){
    return (rand() % (max-min)) + min;
}

double random_angle(){
	// RAND_MAX :    32767
	return 6.283185 * ( rand() / 32767.0 );
}
*/
double lerp(double start, double stop, double amt) {
    return start + (stop-start) * amt;
}

double map(double value, double source_lo, double source_hi,  double dest_lo, double dest_hi) {
	return dest_lo + (dest_hi - dest_lo) * ((value - source_lo) / (source_hi - source_lo));
}

double ellipticalMap(double value, double source_lo, double source_hi, double dest_lo, double dest_hi){
  return dest_hi +((dest_lo-dest_hi)/abs(dest_lo-dest_hi))*SDL_sqrt((1-(sq(value-source_lo)/sq(source_hi-source_lo)))*sq(dest_hi-dest_lo));
}

double sigmoidMap(double value, double source_lo, double source_hi, double dest_lo, double dest_hi){
  return ( (dest_hi-dest_lo) * ( 1 / (1 + SDL_exp( -map( value, source_lo, source_hi, -6, 6 ) ) ) ) ) + dest_lo;
}
double advSigmoidMap(double value, double source_lo, double source_hi, double Slo, double Shi, double dest_lo, double dest_hi){
  return ( (dest_hi-dest_lo) * ( 1 / (1 + SDL_exp( -map( value, source_lo, source_hi, Slo, Shi ) ) ) ) ) + dest_lo;
}

int constrain( int a, int min, int max ){
	if( a < min ) return min;
	else if( a > max ) return max;
	else return a;
}
int cycle( int a, int min, int max ){
	if( a < min ) return max;
	else if( a > max ) return min;
	else return a;
}
float constrainf( float a, float min, float max ){
	if( a < min ) return min;
	else if( a > max ) return max;
	else return a;
}
double constrainD( double a, double min, double max ){
	if( a < min ) return min;
	else if( a > max ) return max;
	else return a;
}

double degrees( double radians ){
	return radians * (double)57.29577951308232087679815481410517033240547246656432154916;//ONE_OVER_PI * 180;
}
double radians( double degrees ){
    return degrees * 0.017453292519943295769236907684886127134428718885417254560;// PI over 180
}

double rectify_angle( double a ){
	if( a < 0 ){
		//printf("++ %f, %f, %f, %f.\n", a, abs(a), abs(a)/TWO_PI, SDL_ceil( abs(a) / TWO_PI ) );
		if( a >= -TWO_PI ) return TWO_PI + a;
		else return (SDL_ceil( abs(a) / TWO_PI ) * TWO_PI) + a;
	}
	else{
		if( a < TWO_PI ) return a;
		else{
			return a - (floor( a / TWO_PI ) * TWO_PI);
		}
	}
}


void strspl( char *string, const char *delimiters, char ***list, int *size ){
	int ss = SDL_strlen( string );
	*list = (char**) SDL_malloc( SDL_ceil(0.5*ss) * sizeof(char*) );
	int sd = SDL_strlen( delimiters );
	*size = 0;

	bool *checks = (bool*) SDL_malloc( ss * sizeof(bool) );
	for( int i = 0; i < ss; ++i ){
		checks[i] = 0;
		for (int j = 0; j < sd; ++j){
			if( string[i] == delimiters[j] ){
				checks[i] = 1;
				break;
			}
		}
	}

	bool looking_for_first = 1;
	for( int i = 0; i < ss; ++i ){
		if( looking_for_first ){
			if( checks[i] ){
				string[i] = '\0';
			}
			else{
				(*list)[0] = string+i;
				(*size)++;
				looking_for_first = 0;
			}
		}
		else{
			if( checks[i] ){
				string[i] = '\0';
				if( i < ss-1 ){
					if( !checks[i+1] ){
						(*list)[*size] = string+i+1;
						(*size)++;
					}
				}
			}
		}
	}
	*list = (char**) realloc( *list, (*size) * sizeof(char*) );
	//apparently for strtok the string MUST be declared as "char string[]" in the calling function
	// it can't be a literal and it can't be "char *string"...
	/* TEST:
	char **list;
	int size = 0;
	char string[] = "split me baby one more time";
	split_string( string, " ", &list, &size );
	for (int i = 0; i < size; ++i){
	    printf("%s\n", list[i] );
	}
	*/
	// char * p = strtok (string, delimiters);
	// int i = 0;
	// while (p != NULL){
	// 	(*list)[i] = p;
	// 	p = strtok (NULL, delimiters);
	// 	++i;
	// }
}

int strcchr( char *string, char C ){ // String Count character
	int i = 0;
	int count = 0;
	while( string[i] != '\0' ){
		if( string[i] == C ) ++count;
	}
	return count;
}

// sub-string
char * substr( char *string, int start, int stop ){
	char *sub = (char*) calloc( stop-start +1, sizeof(char) );
	for (int i = start; i < stop; ++i){
		sub[i-start] = string[i];
	}
	sub[ stop-start ] = '\0';
	return sub;
}

bool str_insert_char( char *string, char C, int pos, int size ){
	char tmpA = string[pos];
	string[pos] = C;
	char tmpB;
	for (int i = pos+1; i < size-1; ++i){
		tmpB = string[i];
		string[i] = tmpA;
		if( string[i] == '\0' ) return 1;
		++i;
		tmpA = string[i];
		string[i] = tmpB;
		if( string[i] == '\0' ) return 1;
	}
	string[size-1] = '\0';
	return 0;
}
void str_delete_char( char *string, int pos, int len ){
	for (int i = pos; i < len; ++i){
	   string[ i ] = string[ i+1 ];
	}
}

char *strtrim( char *string ){
	char *out;
	bool first = 0;
	for (int i = 0; string[i] != '\0' ; ++i){
		if( string[i] == ' ' || string[i] == '\t' || string[i] == '\n' ){
			if( first ){
				string[i] = '\0';
				break;
			}
		}
		else{
			if( !first ){
				out = string + i;
				first = 1;
			}
		}
	}
	return out;
}
void strtrim_trailingNL( char *string ){
	int len = SDL_strlen( string );
	for (int i = len; i >= 0 ; --i){
		if( string[i] == '\n' ){
			string[i] = '\0';
			break;
		}
	}
}

bool fseek_lines( FILE* f, int N ){
	char c = getc( f );
	while( c != EOF ){
		if( c == '\n' ){
			N -= 1;
			if( N == 0 ) return 1;
		}
		c = getc( f );
	}
	return 0;
}

bool fseek_string( FILE *f, char *str ){
	char c = getc( f );
	int i = 0;
	while( c != EOF ){
		if( c == str[i] ){
			i++;
			if( str[i] == '\0' ) return 1;
		}
		else{
			i = 0;
		}
		c = getc( f );
	}
	return 0;
}

Uint16 *ascii_to_unicode( char *str ){
	int len = SDL_strlen( str );
	Uint16 *out = SDL_malloc( len * sizeof(Uint16) );
	int c = 0;
	for( int i = 0; str[i] != '\0'; ++i ){
		if( (str[i] & 0x80) == 0 ){
			out[c] = (Uint16) str[i];
			++c;
		}
		else{
			if( (str[i] & 0xE0 ) == 0xC0 ){
				out[c] = ( ( str[i] & 0x1F ) << 6 ) | (str[i+1] & 0x3F);
				++c;
				++i;
				--len;
			}
			if( (str[i] & 0xF0 ) == 0xE0 ){
				out[c] = ( ( str[i] & 0x0F ) << 12 ) | ( (str[i+1] & 0x3F) << 6 ) | (str[i+2] & 0x3F);
				++c;
				i += 2;
				len -= 2;
			}
		}
	}
	out = realloc( out, (len+1) * sizeof(Uint16) );
	out[len] = '\0';
	return out;
}

bool cursor_in_rect( SDL_Event *event, SDL_Rect *R ){
	switch (event->type) {
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
			return ( event->button.x > R->x && event->button.x < R->x + R->w ) && ( event->button.y > R->y && event->button.y < R->y + R->h );
		case SDL_EVENT_MOUSE_MOTION:
			return ( event->motion.x > R->x && event->motion.x < R->x + R->w ) && ( event->motion.y > R->y && event->motion.y < R->y + R->h );
		default:
			return 0;
	}
}

bool coordinates_in_Rect( float x, float y, SDL_Rect *R ){
	return ( x > R->x && x < R->x + R->w ) && ( y > R->y && y < R->y + R->h );
}
bool coordinates_in_FRect( float x, float y, SDL_FRect *R ){
	return ( x > R->x && x < R->x + R->w ) && ( y > R->y && y < R->y + R->h );
}
bool rect_overlap( SDL_FRect *A, SDL_FRect *B ){
	return ( ( A->x + A->w > B->x ) && ( B->x + B->w > A->x ) ) && ( ( A->y + A->h > B->y ) && ( B->y + B->h > A->y ) );
}
bool intersecting_or_touching( SDL_Rect *A, SDL_Rect *B){
	return ( ( A->x + A->w >= B->x ) && ( B->x + B->w >= A->x ) ) && ( ( A->y + A->h >= B->y ) && ( B->y + B->h >= A->y ) );
}

SDL_Rect add_rects( SDL_Rect *A, SDL_Rect *B){
	SDL_Rect out = *A;
	if( B->x < A->x ) out.x = B->x;
	if( B->y < A->y ) out.y = B->y;
	if( B->x + B->w > A->x + A->w  ) out.w = (B->x + B->w) - A->x;
	if( B->y + B->h > A->y + A->h  ) out.h = (B->y + B->h) - A->y;
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


bool alphanumeric( char c ){
  if( lower_case( c ) || upper_case( c ) || signed_numeral( c ) || c == ' ' ) return 1;
  else return 0;
}
bool lower_case( char c ){
  if( c >= 97 && c <= 122) return 1;
  else return 0;
}
bool upper_case( char c ){
  if( c >= 65 && c <= 90) return 1;
  else return 0;
}
bool numeral( char c ){
  if( ( c >= 48 && c <= 57) || c == '.' ) return 1;
  else return 0;
}
bool signed_numeral( char c ){
  if( ( c >= 48 && c <= 57) || c == '.' || c == '-' || c == '+' ) return 1;
  else return 0;
}
bool punctuation_or_symbol( char c ){
		 if( c >=  32 && c <=  47 )  return 1; //   ! " # $ % & ' ( ) * + , - . /
	else if( c >=  58 && c <=  64 )  return 1; // : ; < = > ? @
	else if( c >=  91 && c <=  96 )  return 1; // [ \ ] ^ _ `
	else if( c >= 123 && c <= 126 )  return 1; // { | } ~
	else return 0;
}
bool ascii_text( char c ){
	if( c >=  32 && c <= 126 )  return 1;
	else return 0;
}
char shifted_keys( char c ){
	//            ! "#$%& '()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
	char S [] = " !\"#$%&\"()*+<_>?)!@#$%^&*(::<+>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}^_`ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~";
	return S[ c - 32 ];
}


int SDL_framerateDelay( int frame_period ){
    //we assume CLOCKS_PER_SEC is 1000, cause it always is...
    static clock_t then = 0;
    clock_t now = clock();
    int elapsed = now - then;
    int delay = frame_period - elapsed;
    //printf("%d - (%d - %d) = %d\n", frame_period, now, then, delay );
    if( delay > 0 ){
    	SDL_Delay( delay );
    	elapsed += delay;
    }
    then = clock();
    return elapsed;
}