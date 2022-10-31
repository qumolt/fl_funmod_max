#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"  
#include <math.h>

#define DFLT_LINEAL 0	//mode 0 lineal
#define DFLT_LISTDUMP 0	//line format
#define DFLT_MAX_BRKPTLIST_SIZE 320

enum INLETS { I_SCALELIST, I_CURVELIST, NUM_INLETS };
enum OUTLETS { O_OUTPUT, NUM_OUTLETS };

typedef struct _fl_brkpt {
	float domain;
	float range;
	float curve;
}fl_brkpt;

typedef struct {

	t_object obj;
	
	short mode;
	short listdump;
	
	float range_lo;
	float range_hi;
	float domain;
	short n_brkpt;

	t_atom *atomlist_out;
	fl_brkpt *brkptlist;
	short lenbrkptlist;
	short max_lenbrkptlist;

	long m_in;
	void *m_proxy;
	void *m_outlet;

} t_fl_funmod;

void *fl_funmod_new(t_symbol *s, short argc, t_atom *argv);
void fl_funmod_assist(t_fl_funmod *x, void *b, long msg, long arg, char *dst);

void fl_funmod_free(t_fl_funmod *x);
void fl_funmod_bang(t_fl_funmod *x);
void fl_funmod_int(t_fl_funmod *x, long n);
void fl_funmod_float(t_fl_funmod *x, double f); 
void fl_funmod_list(t_fl_funmod *x, t_symbol *s, long argc, t_atom *argv);
void fl_funmod_mode(t_fl_funmod *x, t_symbol *s, long argc, t_atom *argv);
void fl_funmod_listdump(t_fl_funmod *x, t_symbol *s, long argc, t_atom *argv);
void fl_funmod_newsize(t_fl_funmod *x, t_symbol *s, long argc, t_atom *argv);

static t_class *fl_funmod_class;