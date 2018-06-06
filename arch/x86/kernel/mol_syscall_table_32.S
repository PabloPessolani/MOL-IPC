ENTRY(mol_sys_call_table)
	.long mol_dc_init		/* 0 */
	.long mol_mini_send		/* 1 */
	.long mol_mini_receive		/* 2 */
	.long mol_void3			/* 3 */
	.long mol_mini_notify		/* 4 */ 
	.long mol_mini_sendrec		/* 5 */ 
	.long mol_mini_rcvrqst		/* 6 */
	.long mol_mini_reply		/* 7 */
	.long mol_dc_end		/* 8 */
	.long mol_bind			/* 9 */
	.long mol_unbind		/* 10 */
	.long mol_dc_dump		/* 11 */
	.long mol_proc_dump		/* 12 */
	.long mol_getpriv		/* 13 get priviledges */
	.long mol_setpriv		/* 14 seg priviledges */
	.long mol_vcopy			/* 15 copy data between user-user or user-kernel */
	.long mol_getdcinfo		/* 16 */
	.long mol_getprocinfo	/* 17 */
	.long mol_void18			/* 18- used for mol_rmtbind */
	.long mol_mini_relay		/* 19 */
	.long mol_proxies_bind		/* 20 */
	.long mol_proxies_unbind	/* 21 */
	.long mol_getnodeinfo		/* 22 */
	.long mol_put2lcl		/* 23 */
	.long mol_get2rmt		/* 24 */
	.long mol_add_node		/* 25 */
	.long mol_del_node		/* 26 */
	.long mol_dvs_init		/* 27 */
	.long mol_dvs_end		/* 28 */
	.long mol_getep			/* 29 */
	.long mol_getdvsinfo		/* 30 */
	.long mol_proxy_conn		/* 31 */
	.long mol_wait4bind		/* 32 */
	.long mol_migrate		/* 33 */
	.long mol_node_up		/* 34 */
	.long mol_node_down		/* 35 */
	.long mol_getproxyinfo	/* 36 */
	.long mol_wakeup		/* 37 */
	


	
	
	







