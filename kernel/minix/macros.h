/* Constants and macros for bit map manipulation. */
#define MAP_CHUNK(map,bit) (map)[((bit)/BITCHUNK_BITS)]
#define CHUNK_OFFSET(bit) ((bit)%BITCHUNK_BITS))
//#define GET_BIT(map,bit) ( MAP_CHUNK(map,bit) & (1 << CHUNK_OFFSET(bit) )
//#define SET_BIT(map,bit) ( MAP_CHUNK(map,bit) |= (1 << CHUNK_OFFSET(bit) )
//#define UNSET_BIT(map,bit) ( MAP_CHUNK(map,bit) &= ~(1 << CHUNK_OFFSET(bit) )

#define get_node_bit(x,y) 	get_sys_bit(x,y)
#define set_node_bit(x,y) 	set_sys_bit(x,y)
#define unset_node_bit(x,y) unset_sys_bit(x,y)

#define get_sys_bit(map,bit) \
	( MAP_CHUNK(map.chunk,bit) & (1 << CHUNK_OFFSET(bit) )
#define set_sys_bit(map,bit) \
	( MAP_CHUNK(map.chunk,bit) |= (1 << CHUNK_OFFSET(bit) )
#define unset_sys_bit(map,bit) \
	( MAP_CHUNK(map.chunk,bit) &= ~(1 << CHUNK_OFFSET(bit) )
#define NR_SYS_CHUNKS	BITMAP_CHUNKS(NR_SYS_PROCS)

#define PRINT_MAP(map) do {\
	MOLDEBUG("%s:", #map);\
	for(i = 0; i < NR_SYS_CHUNKS; i++){\
		MOLDEBUG("%X.",(map)[i]);\
	}\
		MOLDEBUG("%X.",(map)[i]);\
}
