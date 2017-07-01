// Porres 2017

#include "m_pd.h"
#include "math.h"

#define MAX_LIMIT 0x7fffffff

static t_class *ramp_class;

typedef struct _ramp
{
    t_object x_obj;
    double  x_phase;
    float   x_min;
    float   x_max;
    float   x_inc;
    float   x_reset;
    t_float    x_lastin;
    t_inlet  *x_inlet_inc;
    t_inlet  *x_inlet_min;
    t_inlet  *x_inlet_max;
    t_outlet *x_outlet;
} t_ramp;

static void ramp_bang(t_ramp *x)
{
    x->x_phase = x->x_reset;
}

static void ramp_reset(t_ramp *x, t_floatarg f)
{
    x->x_reset = f;
}

static t_int *ramp_perform(t_int *w)
{
    t_ramp *x = (t_ramp *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // trigger
    t_float *in2 = (t_float *)(w[4]); // increment
    t_float *in3 = (t_float *)(w[5]); // min
    t_float *in4 = (t_float *)(w[6]); // max
    t_float *out = (t_float *)(w[7]);
    double phase = x->x_phase;
    float reset = x->x_reset; // reset point
    t_float lastin = x->x_lastin;
    double output;
    while (nblock--)
    {
        float trig = *in1++; // trigger
        double phase_step = *in2++; // phase_step
        float min = *in3++; // min
        float max = *in4++; // max
        if(min == max)
            output = min;
        else
            {
            if (trig > 0 && lastin <= 0)
                phase = reset;
            else
                {
                if(min > max)
                    { // swap values
                        float temp;
                        temp = max;
                        max = min;
                        min = temp;
                    };
                if(phase < min || phase >= max) // wrap phase
                    {
                    float range = max - min;
                    if(phase < min && phase_step < 0)
                        {
                        while(phase < min)
                        phase += range; // wrapped phase
                        }
                    else
                        if(phase > max && phase_step > 0)
                        phase = fmod(phase - min, range) + min; // wrapped phase
                    }
                }
            output = phase;
            lastin = trig;
            }
        *out++ = output;
        phase += phase_step; // next phase
    }
    x->x_phase = phase;
    x->x_lastin = lastin; // last input
    return (w + 8);
}

static void ramp_dsp(t_ramp *x, t_signal **sp)
{
    dsp_add(ramp_perform, 7, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
}

static void *ramp_free(t_ramp *x)
{
    inlet_free(x->x_inlet_inc);
    inlet_free(x->x_inlet_min);
    inlet_free(x->x_inlet_max);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *ramp_new(t_symbol *s, int argc, t_atom *argv)
{
    t_ramp *x = (t_ramp *)pd_new(ramp_class);
///////////////////////////
    x->x_lastin = 0;
    x->x_reset = 0;
    x->x_min = 0.;
    x->x_max = MAX_LIMIT;
    x->x_inc = 1.;
    if(argc)
    {
        int numargs = 0;
        while(argc > 0 )
        {
            if(argv -> a_type == A_FLOAT)
            { // if nullpointer, should be float or int
                switch(numargs)
                {
                    case 0: x->x_inc = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 1: x->x_min = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 2: x->x_max = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 3: x->x_reset = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    default:
                        argc--;
                        argv++;
                        break;
                };
            }
            else // not a float
                goto errstate;
        };
    }
///////////////////////////
    x->x_phase = (double)x->x_reset ;
    x->x_inlet_inc = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_inc, x->x_inc);
    x->x_inlet_min = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_min, x->x_min);
    x->x_inlet_max = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_max, x->x_max);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
    errstate:
        pd_error(x, "ramp~: improper args");
        return NULL;
}

void ramp_tilde_setup(void)
{
    ramp_class = class_new(gensym("ramp~"),
        (t_newmethod)ramp_new, (t_method)ramp_free,
        sizeof(t_ramp), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(ramp_class, nullfn, gensym("signal"), 0);
    class_addmethod(ramp_class, (t_method)ramp_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(ramp_class, (t_method)ramp_bang);
    class_addmethod(ramp_class, (t_method)ramp_reset, gensym("reset"), A_FLOAT, 0);
}