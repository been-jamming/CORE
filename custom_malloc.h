void *custom_malloc(size_t size);
void custom_free(void *pointer);
size_t custom_block_size(void *pointer);
void *custom_realloc(void *pointer, size_t size);
void custom_malloc_abort();
void custom_malloc_init();
void custom_malloc_deinit();

#ifdef USE_CUSTOM_ALLOC
	#define malloc custom_malloc
	#define free custom_free
	#define realloc custom_realloc
#endif
