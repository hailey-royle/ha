#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#define i8 int8_t
#define u8 uint8_t
#define i16 int16_t
#define u16 uint16_t
#define i32 int32_t
#define u32 uint32_t
#define f32 float32_t
#define i64 int64_t
#define u64 uint64_t
#define f64 float64_t

#define ansi_cursor_home "\x1b[H"
#define ansi_erase_line "\x1b[2K"

#define ansi_cursor_show "\x1b[?25h"
#define ansi_cursor_hidden "\x1b[?25l"
#define ansi_start_alt_screen "\x1b[?1049h"
#define ansi_end_alt_screen "\x1b[?1049l"

#define ansi_reset_graphics "\x1b[0m"
#define ansi_bold_start "\x1b[1m"
#define ansi_bold_end "\x1b[22m"
#define ansi_underline_start "\x1b[4m"
#define ansi_underline_end "\x1b[24m"
#define ansi_inverse_start "\x1b[7m"
#define ansi_inverse_end "\x1b[27m"

#define ansi_backrgound_black "\x1b[40m"
#define ansi_foreground_black "\x1b[30m"
#define ansi_foreground_red "\x1b[31m"
#define ansi_backrgound_red "\x1b[41m"
#define ansi_foreground_green "\x1b[32m"
#define ansi_backrgound_green "\x1b[42m"
#define ansi_foreground_yellow "\x1b[33m"
#define ansi_backrgound_yellow "\x1b[43m"
#define ansi_foreground_blue "\x1b[34m"
#define ansi_backrgound_blue "\x1b[44m"
#define ansi_foreground_magenta "\x1b[35m"
#define ansi_backrgound_magenta "\x1b[45m"
#define ansi_foreground_cyan "\x1b[36m"
#define ansi_backrgound_cyan "\x1b[46m"
#define ansi_foreground_white "\x1b[37m"
#define ansi_backrgound_white "\x1b[47m"
#define ansi_foreground_default "\x1b[39m"
#define ansi_backrgound_default "\x1b[49m"

#define array_funcs( type )\
\
void push_ ## type ## _array( type ## _array* array, type push ){\
        if( array->count >= array->allocated ){\
                i32 new_allocated = ( array->allocated == 0 ) ? 16 : array->allocated * 2;\
                array->data = realloc( array->data, sizeof( array->data[ 0 ]) * new_allocated );\
                assert( array->data != NULL );\
                memset( &array->data[ array->allocated ], 0, sizeof( array->data[ 0 ]) * ( new_allocated - new_allocated ));\
                array->allocated = new_allocated;\
        }\
        array->data[ array->count ] = push;\
        array->count += 1;\
}

void assert_failed( char* file, i32 line, const char* func, char* expression ){
	fprintf( stderr, "%s%s:%d:%s%s \"%s\"\r\n", ansi_foreground_red, file, line, func, ansi_foreground_default, expression );
	fflush( stderr );
	exit( 1 );
}

#define assert( expression ){\
	if( !( expression )){\
		assert_failed( __FILE__, __LINE__, __func__, #expression );\
	}\
}

typedef struct {
        char* data;
        i32 count;
        i32 allocated;
} string;

i8 string_from_file( string* source, string name ){
        assert( source != NULL );
        assert( source->data == NULL );
        assert( source->count == 0 );
        assert( source->allocated == 0 );
        assert( name.data != NULL );
        assert( name.count > 0 );
        char name_null[ name.count + 1 ];
        memcpy( name_null, name.data, name.count );
        name_null[ name.count ] = '\0';
        FILE* file = fopen( name_null, "r" );
        if( file == NULL ){
                return 1;
        };
        i32 error = fseek( file, 0, SEEK_END );
        if( error != 0 ){
                return 1;
        };
        i32 file_count = ftell( file );
        rewind( file );
        source->data = malloc( file_count + 1 );
        if( source->data == NULL ){
                return 1;
        };
        i64 bytes_read = fread( source->data, 1, file_count, file );
        if( file_count != bytes_read ){
                return 1;
        };
        fclose( file );
        source->allocated = file_count + 1;
        source->count = file_count;
        source->data[ source->count ] = '\0';
        return 0;
}

i8 string_to_file( string* source, char* file_name, i32 name_length ){
        assert( source != NULL );
        assert( source->allocated > source->count || source->count <= 0 );
        assert( file_name != NULL );
        char name[ name_length + 1 ];
        memcpy( name, file_name, name_length );
        name[ name_length ] = '\0';
        FILE* file = fopen( name, "w" );
        if( file == NULL ){
                return 1;
        }
        i64 bytes_wrote = (i64) fwrite( source->data, 1, source->count, file );
        if( source->count != bytes_wrote ){
                return 1;
        }
        fclose( file );
        return 0;
}

void string_free( string* source ){
        assert( source != NULL );
        if( source->data != NULL ){
                free( source->data );
                source->data = NULL;
        }
        source->allocated = 0;
        source->count = 0;
}

void string_alloc( string* source, i32 count ){
        assert( source != NULL );
        assert( count >= 0 );
        if( source->count + count >= source->allocated ){
                i32 allocated = ( source->allocated + count ) * 2;
                char* tmp = realloc( source->data, allocated );
                assert( tmp != NULL );
                source->data = tmp;
                source->allocated = allocated;
        }
}

void string_append( string* source, char* src, i32 count ){
        assert( source != NULL );
        assert( src != NULL );
        assert( count >= 0 );
        assert( source->allocated > source->count || source->count <= 0 );
        if( source->count + count >= source->allocated ){
                i32 allocated = ( source->allocated + count ) * 2;
                char* tmp = realloc( source->data, allocated );
                assert( tmp != NULL );
                source->data = tmp;
                source->allocated = allocated;
        }
        memmove( &source->data[ source->count ], src, count );
        source->count += count;
        source->data[ source->count ] = '\0';
        return;
}

void string_insert( string* source, i32 index, char* src, i32 count ){
        assert( source != NULL );
        assert( src != NULL );
        assert( count >= 0 );
        assert( source->count >= index );
        assert( source->allocated > source->count || source->count <= 0 );
        if( source->count + count >= source->allocated ){
                i32 allocated = ( source->allocated + count ) * 2;
                char* tmp = realloc( source->data, allocated );
                assert( tmp != NULL );
                source->data = tmp;
                source->allocated = allocated;
        }
        memmove( &source->data[ index + count ], &source->data[ index ], source->count - index );
        memmove( &source->data[ index ], src, count );
        source->count += count;
        source->data[ source->count ] = '\0';
        return;
}

#if defined(_WIN32)
        #error Windows not yet supported.
        //#define OS_WINDOWS 1
#elif defined(__gnu_linux__) || defined(__linux__)
        #define OS_LINUX 1
        #include <unistd.h>
        #include <pthread.h>
#elif defined(__APPLE__) && defined(__MACH__)
        #error Mac not yet supported.
        //#define OS_MAC 1
#else
        #error Unknown operating system.
#endif

#if defined( OS_LINUX )

#define thread pthread_t
#define barrier pthread_barrier_t
#define mutex pthread_mutex_t

i64 result_cpu_count = 0;

i64 find_cpu_count(){
	if( result_cpu_count == 0 ){
	        result_cpu_count = sysconf( _SC_NPROCESSORS_ONLN );
	}
	assert( result_cpu_count > 0 );
	return result_cpu_count;
}

void thread_create( thread* thread, void* (start_routine)( void* ), void* arg ){
        i32 error = pthread_create( thread, NULL, start_routine, arg );
        assert( error == 0 );
}

void thread_join( thread thread, void* thread_return ){
        i32 error = pthread_join( thread, thread_return );
        assert( error == 0 );
}

void barrier_init( pthread_barrier_t* barrier, i32 count ){
        pthread_barrier_init( barrier, NULL, count );
}

void barrier_wait( barrier* barrier ){
        pthread_barrier_wait( barrier );
}

void mutex_init( mutex* mutex ){
        pthread_mutex_init( mutex, NULL );
}

void mutex_lock( mutex* mutex ){
        pthread_mutex_lock( mutex );
}

i32 mutex_trylock( mutex* mutex ){
        return pthread_mutex_trylock( mutex );
}

void mutex_unlock( mutex* mutex ){
        pthread_mutex_unlock( mutex );
}

#endif // linux

