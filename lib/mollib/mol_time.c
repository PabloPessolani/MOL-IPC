#include <mollib.h>


mnx_time_t mol_time(mnx_time_t *tp)
{
	message m __attribute__((aligned(0x1000)));
	message *m_ptr;
	int rcode;

	m_ptr =& m;
	rcode = molsyscall(PM_PROC_NR, MOLTIME, &m);
	LIBDEBUG(MSG2_FORMAT, MSG2_FIELDS(m_ptr));

	if (rcode < 0) return( (mnx_time_t) -1);
	if (tp != (mnx_time_t *) 0) *tp = m.m2_l1;
	return(m.m2_l1);
}
