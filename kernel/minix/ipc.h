#ifndef _IPC_H
#define _IPC_H

#define TIMEOUT_NOWAIT			0	
#define TIMEOUT_FOREVER			(-1)	


/*==========================================================================* 
 * Types relating to messages. 						    *
 *==========================================================================*/ 

#define M1                 1
#define M3                 3
#define M4                 4
#define M3_STRING         14

#define MINIX_MSG	1	/* the payload is a MINIX message */

#define 	SLOTS_BY_MSG		4 /* number of slots that fit in a message */

typedef struct {int m1i1, m1i2, m1i3; char *m1p1, *m1p2, *m1p3;} mess_1;
typedef struct {int m2i1, m2i2, m2i3; long m2l1, m2l2; char *m2p1;} mess_2;
typedef struct {int m3i1, m3i2; char *m3p1; char m3ca1[M3_STRING];} mess_3;
typedef struct {long m4l1, m4l2, m4l3, m4l4, m4l5;} mess_4;
typedef struct {short m5c1, m5c2; int m5i1, m5i2; long m5l1, m5l2, m5l3;}mess_5;
typedef struct {char m6ca1[sizeof(mess_3)];} mess_6;
typedef struct {int m7i1, m7i2, m7i3, m7i4; char *m7p1, *m7p2;} mess_7;
typedef struct {int m8i1, m8i2; char *m8p1, *m8p2, *m8p3, *m8p4;} mess_8;
typedef struct {int m9i1, m9l1; struct timespec m9t1;} mess_9;
typedef struct {int mAdst, mAnr, mAia[SLOTS_BY_MSG];} mess_A;


typedef struct {
  int m_source;			/* who sent the message */
  int m_type;			/* what kind of message is it */
  union {
	mess_1 m_m1;
	mess_2 m_m2;
	mess_3 m_m3;
	mess_4 m_m4;
	mess_5 m_m5;
	mess_6 m_m6;
	mess_7 m_m7;
	mess_8 m_m8;
	mess_9 m_m9;
	mess_A m_mA;
  } m_u;
} message;


/* The following defines provide names for useful members. */
#define m1_i1  m_u.m_m1.m1i1
#define m1_i2  m_u.m_m1.m1i2
#define m1_i3  m_u.m_m1.m1i3
#define m1_p1  m_u.m_m1.m1p1
#define m1_p2  m_u.m_m1.m1p2
#define m1_p3  m_u.m_m1.m1p3

#define m2_i1  m_u.m_m2.m2i1
#define m2_i2  m_u.m_m2.m2i2
#define m2_i3  m_u.m_m2.m2i3
#define m2_l1  m_u.m_m2.m2l1
#define m2_l2  m_u.m_m2.m2l2
#define m2_p1  m_u.m_m2.m2p1

#define m3_i1  m_u.m_m3.m3i1
#define m3_i2  m_u.m_m3.m3i2
#define m3_p1  m_u.m_m3.m3p1
#define m3_ca1 m_u.m_m3.m3ca1

#define m4_l1  m_u.m_m4.m4l1
#define m4_l2  m_u.m_m4.m4l2
#define m4_l3  m_u.m_m4.m4l3
#define m4_l4  m_u.m_m4.m4l4
#define m4_l5  m_u.m_m4.m4l5

#define m5_c1  m_u.m_m5.m5c1
#define m5_c2  m_u.m_m5.m5c2
#define m5_i1  m_u.m_m5.m5i1
#define m5_i2  m_u.m_m5.m5i2
#define m5_l1  m_u.m_m5.m5l1
#define m5_l2  m_u.m_m5.m5l2
#define m5_l3  m_u.m_m5.m5l3

#define m6_ca1 m_u.m_m6.m6ca1

#define m7_i1  m_u.m_m7.m7i1
#define m7_i2  m_u.m_m7.m7i2
#define m7_i3  m_u.m_m7.m7i3
#define m7_i4  m_u.m_m7.m7i4
#define m7_p1  m_u.m_m7.m7p1
#define m7_p2  m_u.m_m7.m7p2

#define m8_i1  m_u.m_m8.m8i1
#define m8_i2  m_u.m_m8.m8i2
#define m8_p1  m_u.m_m8.m8p1
#define m8_p2  m_u.m_m8.m8p2
#define m8_p3  m_u.m_m8.m8p3
#define m8_p4  m_u.m_m8.m8p4

#define m9_i1  m_u.m_m9.m9i1
#define m9_l1  m_u.m_m9.m9l1
#define m9_t1  m_u.m_m9.m9t1

#define mA_nr  m_u.m_mA.mAnr
#define mA_dst m_u.m_mA.mAdst
#define mA_ia  m_u.m_mA.mAia

#define MSG1_FORMAT "source=%d type=%d m1i1=%d m1i2=%d m1i3=%d m1p1=%p m1p2=%p m1p3=%p \n"
#define MSG2_FORMAT "source=%d type=%d m2i1=%d m2i2=%d m2i3=%d m2l1=%ld m2l2=%ld m2p1=%p\n"
#define MSG3_FORMAT "source=%d type=%d m3i1=%d m3i2=%d m3p1=%p m3ca1=[%s]\n"
#define MSG4_FORMAT "source=%d type=%d m4l1=%ld m4l2=%ld m4l3=%ld m4l4=%ld m4l5=%ld\n"
#define MSG5_FORMAT "source=%d type=%d m5c1=%c m5c2=%c m5i1=%d m5i2=%d m5l1=%ld m5l2=%ld m5l3=%ld\n"
#define MSG6_FORMAT "source=%d type=%d m6ca1=[%s]\n"
#define MSG7_FORMAT "source=%d type=%d m7i1=%d m7i2=%d m7i3=%d m7i4=%d m7p1=%p m7p2=%p\n"
#define MSG8_FORMAT "source=%d type=%d m8i1=%d m8i2=%d m8p1=%p m8p2=%p m8p3=%p m8p4=%p\n"
#define MSG9_FORMAT "source=%d type=%d m9i1=%d m9l1=%ld m9t1.tv_sec=%ld m9t1.tv_nsec=%ld\n"
#define MSGA_FORMAT "source=%d type=%d dest=%d mAnr=%d mAia[0]=%d mAia[1]=%d mAia[2]=%d mAia[3]=%d\n"

#define MSG1_FIELDS(p) 	p->m_source,p->m_type, p->m1_i1, p->m1_i2, p->m1_i3, p->m1_p1, p->m1_p2, p->m1_p3
#define MSG2_FIELDS(p) 	p->m_source,p->m_type, p->m2_i1, p->m2_i2, p->m2_i3, p->m2_l1, p->m2_l2, p->m2_p1
#define MSG3_FIELDS(p) 	p->m_source,p->m_type, p->m3_i1, p->m3_i2, p->m3_p1, p->m3_ca1
#define MSG4_FIELDS(p) 	p->m_source,p->m_type, p->m4_l1, p->m4_l2, p->m4_l3, p->m4_l4, p->m4_l5
#define MSG5_FIELDS(p) 	p->m_source,p->m_type, p->m5_c1, p->m5_c2, p->m5_i1, p->m5_i2, p->m5_l1, p->m5_l2, p->m5_l3
#define MSG6_FIELDS(p) 	p->m_source,p->m_type, p->m6_ca1
#define MSG7_FIELDS(p) 	p->m_source,p->m_type, p->m7_i1, p->m7_i2, p->m7_i3, p->m7_i4, p->m7_p1, p->m7_p2
#define MSG8_FIELDS(p) 	p->m_source,p->m_type, p->m8_i1, p->m8_i2, p->m8_p1, p->m8_p2, p->m8_p3, p->m8_p4
#define MSG9_FIELDS(p) 	p->m_source,p->m_type, p->m9_i1, p->m9_l1, p->m9_t1.tv_sec, p->m9_t1.tv_nsec
#define MSGA_FIELDS(p) 	p->m_source,p->m_type, p->mA_dst, p->mA_nr, p->mA_ia[0], p->mA_ia[1], p->mA_ia[2],p->mA_ia[3]

#endif /* _IPC_H */
