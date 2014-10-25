#ifndef GUI_TEXTBOX_H
#define GUI_TEXTBOX_H

#include "gui.h"


struct textlines_iterator {
	/* array of text lines */
	char **textlines;
	/* number of lines */
	int nlines;
	/* current line */
	int k;
	/* current character on the line */
	int m;
};


/* initialize textline iterator to the beginning */
void textlines_iterator_init(struct textlines_iterator *it, char **textlines, int nlines);

/* return the current character, or -1 if at the end */
int textlines_iterator_peekc(struct textlines_iterator *it);

/* advance to the next character; return -1 if at the end */
int textlines_iterator_next(struct textlines_iterator *it);

/* ---------------------------------------------------------------------- */

struct guitextbox {
    /* the window base class; MUST BE THE FIRST ELEMENT */
    struct guiwindow win;
    /* array of strings. NULL strings are ignored */
    char **textlines;			// TODO make this into stringlist
    /* number of positions in textlines */
    int nlines;
    /* interpret formating markup (according to rfc1896) ? */
    int markup;
    /* spread long textlines over several print lines (1),
     * or trim them at the boundary (0) ? */
    int wraplines;
    /* expand escape codes entered using '\' */
    int escexpand;
};


/*
	RFC1896 text/enriched

	Token		Meaning
	<< 			Literal <
	'\n'		Line break
	<param>
	<bold>, <b>
	<italic>, <i>
	<underline>, <u>
	<fixed>, <fi>
	<fontfamily>, <fo>
	<color>
	<smaller>, <sm>
	<bigger>, <bi>
	<center>
	<flushleft>
	<flushright>
	<flushboth>
	<paraindent>
	<nofill>


  BNF:
	
	<echar> ::=
		normal char except '\'
		| '\' <esccode>
	
	<esccode> ::=
		'n' | 'r' | '\' | '"' | {'u' <unicode_seq>}

	<unicode_seq> ::=
		<digit> <digit> <digit>

	<token> ::=
		{'<' '<'}
		| '<' ['/'] <tokname>
		| <echar\{'<'}>

	<tokname> ::=
		{ <echar\{'<','>'}> * } '>'
*/


/* allocate new textbox gui element */
struct guitextbox *gui_textbox_alloc(int nlines);


/* drawing callback for textbox */
int gui_textbox_draw_cb(u8g_t *pu8g, struct guiwindow *win,
                struct guipoint abspos);


void stringlist_scroll_add(struct guitextbox *tbox, char *str);


#endif
