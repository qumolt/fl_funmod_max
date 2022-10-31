#include "flfunmod.h"

void C74_EXPORT main() {

	t_class *c;

	c = class_new("flfunmod",(method)fl_funmod_new, (method)fl_funmod_free,sizeof(t_fl_funmod), 0, 0);
	class_addmethod(c, (method)fl_funmod_assist,"assist", A_CANT, 0);
	class_addmethod(c, (method)fl_funmod_bang, "bang", 0);
	class_addmethod(c, (method)fl_funmod_int, "int", A_LONG, 0);
	class_addmethod(c, (method)fl_funmod_float, "float", A_LONG, 0);
	class_addmethod(c, (method)fl_funmod_list, "list", A_GIMME, 0);

	class_addmethod(c, (method)fl_funmod_mode, "mode", A_GIMME, 0);
	class_addmethod(c, (method)fl_funmod_listdump, "listdump", A_GIMME, 0);
	class_addmethod(c, (method)fl_funmod_newsize, "listsize", A_GIMME, 0);

	class_register(CLASS_BOX, c);
	fl_funmod_class = c; 
}

void *fl_funmod_new(t_symbol *s, short argc, t_atom *argv) 
{
	t_fl_funmod *x = (t_fl_funmod *)object_alloc(fl_funmod_class);

	x->m_proxy = proxy_new((t_object *)x, 1, &x->m_in);
	x->m_outlet = outlet_new((t_object *)x, NULL);

	x->mode = DFLT_LINEAL;
	x->listdump = DFLT_LISTDUMP;
	x->range_lo = -1.;
	x->range_hi = 1.;
	x->domain = 1000.;
	x->n_brkpt = 0;

	x->atomlist_out = (t_atom *)sysmem_newptr(sizeof(t_atom) * 3 * DFLT_MAX_BRKPTLIST_SIZE);
	if (x->atomlist_out) {
		for (long i = 0; i < DFLT_MAX_BRKPTLIST_SIZE; i++) {
			atom_setfloat(x->atomlist_out + i, 0.);
		}
	}
	else{ object_error((t_object *)x, "no memory for output atom array"); }
	x->max_lenbrkptlist = DFLT_MAX_BRKPTLIST_SIZE;
	x->lenbrkptlist = 0;
	
	x->brkptlist = (fl_brkpt *)sysmem_newptr(sizeof(fl_brkpt) * DFLT_MAX_BRKPTLIST_SIZE);
	if (x->brkptlist) {
		for (long i = 0; i < DFLT_MAX_BRKPTLIST_SIZE; i++) {
			x->brkptlist[i].domain = 0;
			x->brkptlist[i].range = 0.;
			x->brkptlist[i].curve = 0.;
		}
	}
	else { object_error((t_object *)x, "no memory to store list"); }
	
	return x;
}

void fl_funmod_assist(t_fl_funmod *x, void *b, long msg, long arg, char *dst)
{
	if (msg == ASSIST_INLET) {										
		switch (arg) {
		case I_SCALELIST:
			sprintf(dst, "(list) domain and range scale");
			break;
		case I_CURVELIST:
			sprintf(dst, "(list) function list");
			break;
		}
	}
	else if (msg == ASSIST_OUTLET) {    
		switch (arg) {
		case O_OUTPUT: 
			sprintf(dst, "(list) scaled function list"); 
			break;
		}
	}
}

void fl_funmod_free(t_fl_funmod *x)
{
	object_free(x->atomlist_out);
	object_free(x->brkptlist);
}

void fl_funmod_bang(t_fl_funmod *x)
{
	short mode = x->mode;
	short listdump = x->listdump;
	long max_list_allowed = x->max_lenbrkptlist;
	short list_stored = x->lenbrkptlist ? 1 : 0;
	object_post((t_object *)x, "mode: %d", mode);
	object_post((t_object *)x, "dumplist: %d", listdump);
	object_post((t_object *)x, "maximum list size allowed: %d", max_list_allowed);
	object_post((t_object *)x, "list stored? %d", list_stored);
}

void fl_funmod_int(t_fl_funmod *x, long n) {}

void fl_funmod_float(t_fl_funmod *x, double f) {}

void fl_funmod_list(t_fl_funmod *x, t_symbol *s, long argc, t_atom *argv)
{
	long ac = argc;
	t_atom *ap = argv;

	float new_domain;
	float new_range_hi;
	float new_range_lo;

	short mode = x->mode;
	short brkpt_size = mode ? 3 : 2; //mode 0:linear (f,f) //mode 1:curve (f,f,f)
	short format = x->listdump; //0:line-curve format (y,dx)(y,dx,c)  //1:listdump format (x,y)(x,y,c)

	float domain = x->domain;
	float range_lo = x->range_lo;
	float range_hi = x->range_hi;
	short n_brkpt = x->n_brkpt;

	float mod_domain;
	float mod_range;
	float curve;

	short lencurvelist = x->lenbrkptlist;
	short max_lencurvelist = x->max_lenbrkptlist;

	float factor = 1.;

	t_atom *atomlist_out = x->atomlist_out;

	switch (proxy_getinlet((t_object *)x)) {
	
	case I_SCALELIST:
		if (ac != 3) { object_error((t_object *)x, "domain [1 arg] and range [2 args] (int float float)"); }

		if (atom_gettype(ap) != A_FLOAT && atom_gettype(ap) != A_LONG &&
			atom_gettype(ap + 1) != A_FLOAT && atom_gettype(ap + 1) != A_LONG &&
			atom_gettype(ap + 2) != A_FLOAT && atom_gettype(ap + 2) != A_LONG) {
			object_error((t_object *)x, "domain [1 arg] and range [2 args] both arguments must be numbers");
		}
		if (!lencurvelist) { object_error((t_object *)x, "list is empty"); return; }

		new_domain = (float)atom_getfloat(ap);
		new_range_lo = (float)MIN(atom_getfloat(ap + 1), atom_getfloat(ap + 2));
		new_range_hi = (float)MAX(atom_getfloat(ap + 1), atom_getfloat(ap + 2));

		if (new_domain < 1) { object_error((t_object *)x, "domain (float) must be a positive number"); return; }

		factor = (float)(new_domain * (1.0 / domain));

		//mode 0:linear (f,f) 
		//mode 1:curve (f,f,f)
		//format 0:line-curve format (y,dx)(y,dx,c)
		//format 1:listdump format (x,y)(x,y,c)
		for (long i = 0; i < n_brkpt; i++) {
			mod_domain = (float)(x->brkptlist[i].domain * factor);
			mod_range = (x->brkptlist[i].range - range_lo) * ((new_range_hi - new_range_lo) / (range_hi - range_lo)) + new_range_lo;
			if (mode) { curve = x->brkptlist[i].curve; }

			if (format) {
				atom_setfloat(atomlist_out + (i * brkpt_size), mod_domain);
				atom_setfloat(atomlist_out + (i * brkpt_size + 1), mod_range);
			}
			else {
				atom_setfloat(atomlist_out + (i * brkpt_size + 1), mod_domain);
				atom_setfloat(atomlist_out + (i * brkpt_size), mod_range);
			}
			if (mode) { atom_setfloat(atomlist_out + (i * brkpt_size + 2), curve); }
		}

		outlet_list(x->m_outlet, NULL, lencurvelist, atomlist_out);

		break;
		
	case I_CURVELIST:
		if (ac >= max_lencurvelist) { object_error((t_object *)x, "maximum list size is %d", max_lencurvelist); return; }
		
		if (ac % brkpt_size != 0) { object_error((t_object *)x, "list size must be multiple of %d", brkpt_size); return; }
		
		n_brkpt = (short)ac / brkpt_size;
		if (format) { //listdump: x mod3(n)=0; y mod3(n)=1; c mod3(n)=2;
			domain = (float)atom_getfloat(ap + ac - brkpt_size); //ac-1=c, ac-2=y, ac-3=x //ac-1=y, ac-2=x 
			range_lo = range_hi = (float)atom_getfloat(ap + 1);
			for (long i = 0; i < n_brkpt; i++) {
				long j = brkpt_size * i;
				range_lo = (float)MIN(range_lo, (double)atom_getfloat(ap + 1 + j));
				range_hi = (float)MAX(range_hi, (double)atom_getfloat(ap + 1 + j));

				x->brkptlist[i].domain = (float)atom_getfloat(ap + j);
				x->brkptlist[i].range = (float)atom_getfloat(ap + j + 1);
				if (mode) { x->brkptlist[i].curve = (float)atom_getfloat(ap + j + 2); }
			}
		}
		else { //linecurve: y mod3(n)=0; x mod3(n)=1; c mod3(n)=2;
			domain = 0;
			range_lo = range_hi = (float)atom_getfloat(ap + 1);
			for (long i = 0; i < n_brkpt; i++) {
				long j = brkpt_size * i;
				range_lo = (float)MIN(range_lo, (double)atom_getfloat(ap + j));
				range_hi = (float)MAX(range_hi, (double)atom_getfloat(ap + j));
				domain += (float)atom_getfloat(ap + j + 1);

				x->brkptlist[i].range = (float)atom_getfloat(ap + j);
				x->brkptlist[i].domain = (float)atom_getfloat(ap + j + 1);
				if (mode) { x->brkptlist[i].curve = (float)atom_getfloat(ap + j + 2); }
			}
		}

		x->n_brkpt = n_brkpt;
		x->domain = domain;
		x->range_hi = range_hi;
		x->range_lo = range_lo;
		x->lenbrkptlist = brkpt_size * n_brkpt;

		break;

	default:
		break;
	}
}

void fl_funmod_mode(t_fl_funmod *x, t_symbol *s, long argc, t_atom *argv)
{
	short mode = 0;
	long ac = argc;
	t_atom *av = argv;
	
	if (!ac) { object_error((t_object *)x, "must have an argument"); return; }
	if (atom_gettype(av) != A_LONG && atom_gettype(av) != A_FLOAT) { object_error((t_object *)x, "argument must be a number"); return; }
	mode = (short)atom_getlong(av);

	x->mode = mode ? 1 : 0;
}

void fl_funmod_listdump(t_fl_funmod* x, t_symbol* s, long argc, t_atom* argv)
{
	short listdump = 0;
	long ac = argc;
	t_atom* av = argv;

	if (!ac) { object_error((t_object*)x, "must have an argument"); return; }
	if (atom_gettype(av) != A_LONG && atom_gettype(av) != A_FLOAT) { object_error((t_object*)x, "argument must be a number"); return; }
	listdump = (short)atom_getlong(av);

	x->listdump = listdump ? 1 : 0;
}

void fl_funmod_newsize(t_fl_funmod *x, t_symbol *s, long argc, t_atom *argv)
{
	short newsize;
	long ac = argc;
	t_atom *av = argv;

	if (!ac) { object_error((t_object *)x, "must have an argument"); return; }
	if (atom_gettype(av) != A_LONG) { object_error((t_object *)x, "argument must be an integer"); return; }
	newsize = (short)atom_getlong(av);

	if (newsize < 1) { object_error((t_object *)x, "argument must be a positive number"); return; }

	x->atomlist_out = (t_atom *)sysmem_resizeptr(x->atomlist_out, newsize * 3 * sizeof(t_atom));
	if (x->atomlist_out) {
		for (long i = 0; i < newsize; i++) {
			atom_setfloat(x->atomlist_out + i, 0.);
		}
	}
	else { object_error((t_object *)x, "no memory for output atom list"); }
	
	x->brkptlist = (fl_brkpt *)sysmem_resizeptr(x->brkptlist, newsize * sizeof(fl_brkpt));
	if (x->brkptlist) {
		for (long i = 0; i < newsize; i++) {
			x->brkptlist[i].domain = 0;
			x->brkptlist[i].range = 0.;
			x->brkptlist[i].curve = 0.;
		}
	}
	else { object_error((t_object *)x, "no memory to store list"); }
	
	x->max_lenbrkptlist = newsize;
	x->lenbrkptlist = 0;
	
	fl_funmod_bang(x);
}