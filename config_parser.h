enum option_type {
	STRING_TYPE,
	INT_TYPE,
	STRINGLIST_TYPE
};

typedef enum option_type option_type;

int parse_config_file(const char *filename);
