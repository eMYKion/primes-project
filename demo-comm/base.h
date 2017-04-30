/* @(#)base.h	1.1 04/10/17 */

extern long long ipg_systime(void);
extern double    ipg_dsystime(void);
extern void      ipg_systime_delay(int msec);
extern void      ipg_sysdelay(long msec);

extern void      ipg_trim_right(char *str);
extern void      ipg_trim_n_right(char *str, int length);
extern void      ipg_trim_left(char *s);
extern int       ipg_trim_space(char *s);
extern char     *ipg_replace_newlines(char *str);
extern int       ipg_next_token(char *in_str, char *out_str, int which, char **rest);
extern char     *ipg_string_for_display(char *str, int max_per_line, int *num_lines);

/*polynom_fit.c */
extern int       polynom_fit(double *x, double *y, double *W, int Xmax, int Nmax);
extern double    polynom(double x, int Nmax);
extern double    polynom_koeff(int n);
