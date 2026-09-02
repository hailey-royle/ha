#include "base.c"

enum syntax_token_kind {
	error_token = 0,
	argument_token,
	proc_token,
	ret_token,
	identifier_token,
	number_token,
	argument_start_token,
	argument_end_token,
	scope_start_token,
	scope_end_token,
	assignment_token,
	pointer_token,
};

typedef struct {
	i32 index;
	enum syntax_token_kind kind;
} syntax_token;

typedef struct {
	syntax_token* data;
	i32 count;
	i32 allocated;
} syntax_token_array;

array_funcs( syntax_token )

typedef struct {
	syntax_token_array* data;
	i32 count;
	i32 allocated;
} syntax_token_array_array;

array_funcs( syntax_token_array )

typedef struct {
	syntax_token_array_array proc;
	string name;
	string input;
	string output;
	i32 index;
} source_file;

typedef struct {
	source_file* data;
	i32 count;
	i32 allocated;
	mutex lock;
	_Atomic i32 parse_index;
	_Atomic i32 parse_finished;
	_Atomic i32 validate_index;
	_Atomic i32 validate_finished;
	_Atomic i32 output_index;
	_Atomic i32 output_finished;
} source_file_array;

array_funcs( source_file )

source_file_array source = { 0 };

void add_file( string name ){
	assert( name.data != NULL );
	assert( name.count > 0 );
	assert( name.allocated == 0 );
	mutex_lock( &source.lock );
	source_file new_file = { 0 };
	new_file.name = name;
	push_source_file_array( &source, new_file );
	mutex_unlock( &source.lock );
}

char* token_kind_string( syntax_token token ){
	switch( token.kind ){
		case error_token: return "error";
		case argument_token: return "argument";
		case proc_token: return "proc";
		case ret_token: return "ret";
		case identifier_token: return "identifier";
		case number_token: return "number";
		case argument_start_token: return "argument_start";
		case argument_end_token: return "argument_end";
		case scope_start_token: return "scope_start";
		case scope_end_token: return "scope_end";
		case assignment_token: return "assignment";
		case pointer_token: return "pointer";
		default: assert( 0 );
	}
	return NULL;
}

void print_syntax_chache(){
	printf( "Syntax Cache\n" );
	for( i32 i = 0; i < source.count; i++ ){
		printf( "  %sfile%s: %.*s\n", ansi_foreground_green, ansi_foreground_default, source.data[ i ].name.count, source.data[ i ].name.data );
		for( i32 j = 0; j < source.data[ i ].proc.count; j++ ){
			syntax_token token = source.data[ i ].proc.data[ j ].data[ 0 ];
			assert( token.kind == proc_token );
			char* syntax_start = &source.data[ i ].input.data[ token.index ];
			char* line_end = strchr( syntax_start, '\n' );
			printf( "    %sproc%s: %.*s\n", ansi_foreground_green, ansi_foreground_default, (i32)( line_end - syntax_start ), syntax_start );
			for( i32 h = 1; h < source.data[ i ].proc.data[ j ].count; h++ ){
				token = source.data[ i ].proc.data[ j ].data[ h ];
				syntax_start = &source.data[ i ].input.data[ token.index ];
				line_end = strchr( syntax_start, '\n' );
				char* token_string = token_kind_string( token );
				if( token.kind == error_token || token.kind == identifier_token ){
					printf( "      %s%s%s: %.*s\n", ansi_foreground_red, token_string, ansi_foreground_default, (i32)( line_end - syntax_start ), syntax_start );
				} else {
					printf( "      %s%s%s: %.*s\n", ansi_foreground_green, token_string, ansi_foreground_default, (i32)( line_end - syntax_start ), syntax_start );
				}
			}
		}
	}
}

i8 is_white_space( char c ){
	return c == ' ' || c == '\t' || c == '\n';
}

i8 is_identifier_start( char c ){
	return ( c >= 'A' && c <= 'Z') || ( c >= 'a' && c <= 'z' ) || c == '_';
}

i8 is_identifier( char c ){
	return ( c >= 'A' && c <= 'Z') || ( c >= 'a' && c <= 'z' ) || ( c >= '0' && c <= '9' ) || c == '_';
}

i8 is_integer_start( char c ){
	return c >= '1' && c <= '9';
}

i8 is_integer( char c ){
	return ( c >= '0' && c <= '9' ) || c == '_';
}

i32 token_length( char* c ){
	char* start = c;
	if( is_identifier_start( *c )){
		do {
			c += 1;
		} while( is_identifier( *c ));
	} else if( is_integer_start( *c )){
		do {
			c += 1;
		} while( is_integer( *c ));
	} else if( *c == '=' ){
		return 1;
	} else if( *c == '[' ){
		return 1;
	} else if( *c == ']' ){
		return 1;
	} else if( *c == '{' ){
		return 1;
	} else if( *c == '}' ){
		return 1;
	} else if( *c == '^' ){
		return 1;
	} else {
		return 0;
	}
	return c - start;
}

void compiler_error( i32 file, i32 index, char* format, ... ){
	assert( source.count > file );
	assert( source.data[ file ].input.count > index );
	fprintf( stderr, "%sError%s ", ansi_foreground_red, ansi_foreground_default );
	{
		va_list args;
		va_start( args, format );
		vfprintf( stderr, format, args );
		va_end( args );
	}
	char* line_start = source.data[ file ].input.data;
	i32 index_line = 1;
	for( i32 i = 0; i < index; i++ ){
		if( source.data[ file ].input.data[ i ] == '\n' ){
			index_line += 1;
			line_start = &source.data[ file ].input.data[ i ];
		}
	}
	i32 line_pre = &source.data[ file ].input.data[ index ] - line_start;
	i32 line_error = token_length( &source.data[ file ].input.data[ index ]);
	i32 line_post = 0;
	while( 1 ){
		if( source.data[ file ].input.data[ index + line_error + line_post ] == '\0' || source.data[ file ].input.data[ index + line_error + line_post ] == '\n' ){
			break;
		}
		line_post += 1;
	}
//  file.ha:line | line of code
	fprintf( stderr, "\n  %.*s:%d | %.*s%s%.*s%s%.*s\n", 
		source.data[ file ].name.count, source.data[ file ].name.data, index_line,
		line_pre, line_start, ansi_foreground_red,
		line_error, line_start + line_pre, ansi_foreground_default,
		line_post, line_start + line_pre + line_error );
	exit( 1 );
}

i8 token_equals( char* a, char* b ){
	if( is_identifier_start( *a )){
		do {
			if( *a != *b ){
				return 0;
			}
			a += 1;
			b += 1;
		} while( is_identifier( *a ));
	}
	if( is_identifier( *b )){
		return 0;
	}
	return 1;
}

syntax_token next_token( i32 file ){
#define c source.data[ file ].input.data[ source.data[ file ].index ]
	assert( source.count > file );
	syntax_token token = { 0 };
	while( 1 ){
		if( is_white_space( c )){
			source.data[ file ].index += 1;
		} else if( c == '\0' ){
			token.index = source.data[ file ].input.count - 1;
			return token;
		} else {
			break;
		}
	}
	token.index = source.data[ file ].index;
	if( is_identifier_start( c )){
		do {
			source.data[ file ].index += 1;
		} while( is_identifier( c ));
		if       ( token_equals( &source.data[ file ].input.data[ token.index ], "proc" )){
			token.kind = proc_token;
		} else if( token_equals( &source.data[ file ].input.data[ token.index ], "ret" )){
			token.kind = ret_token;
		} else {
			token.kind = identifier_token;
		}
	} else if( is_integer_start( c )){
		do {
			source.data[ file ].index += 1;
		} while( is_integer( c ));
		token.kind = number_token;
	} else if( c == '=' ){
		source.data[ file ].index += 1;
		token.kind = assignment_token;
	} else if( c == '[' ){
		source.data[ file ].index += 1;
		token.kind = argument_start_token;
	} else if( c == ']' ){
		source.data[ file ].index += 1;
		token.kind = argument_end_token;
	} else if( c == '{' ){
		source.data[ file ].index += 1;
		token.kind = scope_start_token;
	} else if( c == '}' ){
		source.data[ file ].index += 1;
		token.kind = scope_end_token;
	} else if( c == '^' ){
		source.data[ file ].index += 1;
		token.kind = pointer_token;
	} else {
		compiler_error( file, source.data[ file ].index, "Invalid syntax." );
	}
	return token;
#undef c
}

void parse_file( i32 file ){
	assert( source.count > file );
	i8 error = string_from_file( &source.data[ file ].input, source.data[ file ].name );
	if( error ){
		fprintf( stderr, "Could not read file \"%.*s\"\n", source.data[ file ].name.count, source.data[ file ].name.data );
		exit( 1 );
	}
	syntax_token token = next_token( file );
	while( 1 ){
		if( token.kind != identifier_token ){
			compiler_error( file, token.index, "Expected procedure name." );
		}
		syntax_token global_scope = token;
		token = next_token( file );
		if( token.kind == proc_token ){
			global_scope.kind = proc_token;
			push_syntax_token_array_array( &source.data[ file ].proc, (syntax_token_array) { 0 });
			push_syntax_token_array( &source.data[ file ].proc.data[ source.data[ file ].proc.count - 1 ], global_scope );
			token = next_token( file );
			if( token.kind != argument_start_token ){
				compiler_error( file, token.index, "Expected procedure type." );
			}
			token = next_token( file );
			while( 1 ){
				if( token.kind == argument_end_token ){
					break;
				}
				if( token.kind != identifier_token ){
					compiler_error( file, token.index, "Expected argument name." );
				}
				token.kind = argument_token;
				push_syntax_token_array( &source.data[ file ].proc.data[ source.data[ file ].proc.count - 1 ], token );
				token = next_token( file );
				while( 1 ){
					if( token.kind == identifier_token ){
						break;
					}
					if( token.kind != pointer_token ){
						compiler_error( file, token.index, "Expected type." );
					}
					token = next_token( file );
				}
				token = next_token( file );
			}
			token = next_token( file );
			while( 1 ){
				if( token.kind == scope_start_token ){
					break;
				}
				while( 1 ){
					if( token.kind == identifier_token ){
						break;
					}
					if( token.kind != pointer_token ){
						compiler_error( file, token.index, "Expected type." );
					}
					token = next_token( file );
				}
				token = next_token( file );
			}
			token = next_token( file );
			if( token.kind != ret_token ){
				compiler_error( file, token.index, "Expected ret." );
			}
			push_syntax_token_array( &source.data[ file ].proc.data[ source.data[ file ].proc.count - 1 ], token );
			token = next_token( file );
			if( token.kind != number_token ){
				compiler_error( file, token.index, "Expected ret value." );
			}
			token = next_token( file );
			if( token.kind != scope_end_token ){
				compiler_error( file, token.index, "Expected scope end." );
			}
		} else {
			compiler_error( file, token.index, "Expected global type.");
		}
		token = next_token( file );
		if( token.kind == error_token ){
			break;
		}
	}
	source.parse_finished += 1;
}

void validate_type( i32 file, char* type ){
	assert( type != NULL );
	while( *type == '^' ){
		type += 1;
	}
	if( token_equals( type, "i32" )){
		return;
	} else if( token_equals( type, "i8" )){
		return;
	}
	// rest of built in, user types
	compiler_error( file, type - source.data[ file ].input.data, "Type not recognized." );
}

void validate_value( i32 file, char* value ){
	assert( value != NULL );
	if( !is_integer_start( *value )){
		compiler_error( file, value - source.data[ file ].input.data, "Not valid value." );
	}
}

void validate_file( i32 file ){
	assert( source.count > file );
	for( i32 i = 0; i < source.data[ file ].proc.count; i++ ){
		{ // TODO multiple return types
			char* proc_type_start = strchr( &source.data[ file ].input.data[ source.data[ file ].proc.data[ i ].data[ 0 ].index ], ']' ) + 1;
			while( is_white_space( *proc_type_start )){
				proc_type_start += 1;
			}
			validate_type( file, proc_type_start );
		}
		for( i32 j = 1; j < source.data[ file ].proc.data[ i ].count; j++ ){
			syntax_token token = source.data[ file ].proc.data[ i ].data[ j ];
			char* next_token = &source.data[ file ].input.data[ token.index ];
			next_token += token_length( &source.data[ file ].input.data[ token.index ] );
			while( is_white_space( *next_token )){
				next_token += 1;
			}
			if( token.kind == argument_token ){
				validate_type( file, next_token );
			} else if( token.kind == ret_token ){
				validate_value( file, next_token );
			} else {
				compiler_error( file, next_token - source.data[ file ].input.data, "Invalid token." );
			}
		}
		break;
	}
	source.validate_finished += 1;
}

void output_file( i32 file ){
	assert( source.count > file );
	assert( source.data[ file ].proc.count == 1 );
	for( i32 i = 0; i < source.data[ file ].proc.count; i++ ){
		string* output = &source.data[ file ].output;
		string_append( output, "
		for( i32 j = 1; j < source.data[ file ].proc.data[ i ].count; j++ ){
			syntax_token token = source.data[ file ].proc.data[ i ].data[ j ];
			char* next_token = &source.data[ file ].input.data[ token.index ];
			next_token += token_length( &source.data[ file ].input.data[ token.index ] );
			while( is_white_space( *next_token )){
				next_token += 1;
			}
			if( token.kind == argument_token ){
			} else if( token.kind == ret_token ){
				validate_value( file, next_token );
			} else {
				compiler_error( file, next_token - source.data[ file ].input.data, "Invalid token." );
			}
		}
		break;
	}
	source.output_finished += 1;
}

void* thread_main( void* argt ){
	i64 thread_index = (i64) argt;
	while( 1 ){
		if( mutex_trylock( &source.lock ) == 0 ){
			assert( source.count >= source.parse_index );
			assert( source.count >= source.parse_finished );
			assert( source.parse_index >= source.parse_finished );
			i32 index = -1;
			if( source.parse_index < source.count ){
				index = source.parse_index;
				source.parse_index++;
			} else if( source.parse_finished == source.parse_index ){ // will only happen after everything is parsed
				mutex_unlock( &source.lock );
				break;
			}
			mutex_unlock( &source.lock );
			if( index >= 0 ){
				parse_file( index );
			}
		}
	}
	while( 1 ){
		if( mutex_trylock( &source.lock ) == 0 ){
			assert( source.count >= source.validate_index );
			assert( source.count >= source.validate_finished );
			assert( source.validate_index >= source.validate_finished );
			i32 index = -1;
			if( source.validate_index < source.count ){
				index = source.validate_index;
				source.validate_index++;
			} else if( source.validate_finished == source.validate_index ){ // will only happen after everything is validated
				mutex_unlock( &source.lock );
				break;
			}
			mutex_unlock( &source.lock );
			if( index >= 0 ){
				validate_file( index );
			}
		}
	}
	// interpreter
	// optimizer
	while( 1 ){
		if( mutex_trylock( &source.lock ) == 0 ){
			assert( source.count >= source.output_index );
			assert( source.count >= source.output_finished );
			assert( source.output_index >= source.output_finished );
			i32 index = -1;
			if( source.output_index < source.count ){
				index = source.output_index;
				source.output_index++;
			} else if( source.output_finished == source.output_index ){ // will only happen after everything is outputd
				mutex_unlock( &source.lock );
				break;
			}
			mutex_unlock( &source.lock );
			if( index >= 0 ){
				output_file( index );
			}
		}
	}
	return NULL;
}

i32 main( i32 argc, char* argv[] ){
	if( argc != 2 ){
		printf( "Usage:$ ha path/to/file.ha\n" );
		exit( 1 );
	}
	add_file( (string){ argv[ 1 ], strlen( argv[ 1 ]), 0 });
	i64 cpu_count = find_cpu_count();
	thread thread_array[ cpu_count - 1 ];
	for( i64 i = 0; i < cpu_count - 1; i++ ){
		thread_create( &thread_array[ i ], thread_main, (void*) i );
	}
	thread_main( (void*) ( cpu_count - 1 ));
	for( i32 i = 0; i < cpu_count - 1; i++ ){
		thread_join( thread_array[ i ], NULL );
	}
	print_syntax_chache();
	return 0;
}

