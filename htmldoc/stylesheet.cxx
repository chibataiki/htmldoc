//
// "$Id: stylesheet.cxx,v 1.3 2002/02/08 19:39:51 mike Exp $"
//
//   CSS sheet routines for HTMLDOC, a HTML document processing program.
//
//   Copyright 1997-2002 by Easy Software Products.
//
//   These coded instructions, statements, and computer programs are the
//   property of Easy Software Products and are protected by Federal
//   copyright law.  Distribution and use rights are outlined in the file
//   "COPYING.txt" which should have been included with this file.  If this
//   file is missing or damaged please contact Easy Software Products
//   at:
//
//       Attn: ESP Licensing Information
//       Easy Software Products
//       44141 Airport View Drive, Suite 204
//       Hollywood, Maryland 20636-3111 USA
//
//       Voice: (301) 373-9600
//       EMail: info@easysw.com
//         WWW: http://www.easysw.com
//
// Contents:
//
//

//
// Include necessary headers.
//

#include "htmldoc.h"
#include "hdstring.h"
//#define DEBUG


//
// 'hdStyleSheet::hdStyleSheet()' - Create a new stylesheet.
//

hdStyleSheet::hdStyleSheet()
{
  // Initialize the stylesheet structure.  Using memset() is safe
  // on structures...

  memset(this, 0, sizeof(hdStyleSheet));

  memset(elements, -1, sizeof(elements));

  ppi = 80.0f;

  // Set the default page to "Universal" with half-inch margins all the
  // way around...

  set_size(595.0f, 792.0f);
  set_margins(36.0f, 36.0f, 36.0f, 36.0f);
}


//
// 'hdStyleSheet::~hdStyleSheet()' - Destroy a stylesheet.
//

hdStyleSheet::~hdStyleSheet()
{
  int		i, j;		// Looping vars
  hdStyle	**s;		// Current style


  // Free all styles...
  if (alloc_styles)
  {
    for (i = num_styles, s = styles; i > 0; i --, s ++)
      delete *s;

    delete[] styles;
  }

  // Free all fonts...
  for (i = 0; i < num_fonts; i ++)
    for (j = 0; j < HD_FONTINTERNAL_MAX; j ++)
      if (fonts[i][j])
        delete fonts[i][j];

  // Free all glyphs...
  if (charset)
    free(charset);

  for (i = 0; i < num_glyphs; i ++)
    if (glyphs[i])
      free(glyphs[i]);

  delete[] glyphs;
}


//
// 'hdStyleSheet::add_style()' - Add a style to a stylesheet.
//

void
hdStyleSheet::add_style(hdStyle *s)	// I - New style
{
  int		i, j, k;		// Looping vars
  hdStyle	**temp;			// New style pointer array
  hdElement	e, e2;			// Elements for new style...


#ifdef DEBUG
  printf("add_style(%p): %s", s, hdTree::elements[s->selectors[0].element]);
  if  (s->selectors[0].pseudo)
    printf(":%s\n", s->selectors[0].pseudo);
  else
    putchar('\n');
#endif // DEBUG

  // Allocate more memory as needed...
  if (num_styles >= alloc_styles)
  {
    temp = new hdStyle *[alloc_styles + 32];

    if (alloc_styles)
    {
      memcpy(temp, styles, alloc_styles * sizeof(hdStyle *));
      delete[] styles;
    }

    alloc_styles += 32;
    styles       = temp;
  }

  // Cache the primary selector element...
  e = s->selectors[0].element;

  // Find where to insert the style...
  if (elements[e] >= 0)
    i = elements[e];	 // Already added this element to the table...
  else if (num_styles)
  {
    // Do a binary search for a group of styles for this element...
    for (i = 0, j = num_styles - 1; i <= j;)
    {
      // Check the element against the left/right styles...
      if (e <= styles[i]->selectors[0].element)
        break; // Insert before left style...

      if (e > styles[j]->selectors[0].element)
      {
        // Insert after right style...
	i = j + 1;
	break;
      }

      // Check the midpoint...
      k  = (i + j) / 2;
      e2 = styles[k]->selectors[0].element;

      if (e < e2)
        j = k - 1;
      else if (e > e2)
        i = k + 1;
      else
      {
        // The midpoint is the right index...
        i = k;
        break;
      }
    }

    elements[e] = i;
  }
  else
    i = elements[e] = 0;

  // Now do the insert...
#ifdef DEBUG
  printf("    inserting at %d, num_styles = %d\n", i, num_styles);
#endif // DEBUG

  if (i < num_styles)
    memmove(styles + i + 1, styles + i, (num_styles - i) * sizeof(hdStyle *));

  styles[i] = s;

  num_styles ++;

  // And update any indices in the elements array...
  for (j = 0; j < HD_ELEMENT_MAX; j ++)
    if (elements[j] >= i && j != e)
      elements[j] ++;

  // Finally, update the max selectors value for this element, to make
  // lookups faster...
  if (s->num_selectors > max_selectors[e])
    max_selectors[e] = s->num_selectors;

#ifdef DEBUG
  printf("    max_selectors = %d\n", max_selectors[e]);
#endif // DEBUG
}


//
// 'hdStyleSheet::find_font()' - Find a font for the given style.
//

hdStyleFont *				// O - Font record
hdStyleSheet::find_font(hdStyle *s)	// I - Style record
{
  return ((hdStyleFont *)0);
}


//
// 'hdStyleSheet::find_style()' - Find the default style for the given
//                                tree node.
//

hdStyle *				// O - Style record
hdStyleSheet::find_style(hdTree *t)	// I - Tree node
{
  int		i;			// Looping var...
  int		nsels;			// Number of selectors...
  hdSelector	sels[HD_SELECTOR_MAX];	// Selectors...
  hdTree	*p;			// Tree pointer...


  // Figure out how many selectors to use...
  if (max_selectors[t->element] > HD_SELECTOR_MAX)
    nsels = HD_SELECTOR_MAX;
  else
    nsels = max_selectors[t->element];

  // Build the selectors for this node...
  for (i = 0, p = t; i < nsels; i ++, p = t->parent)
  {
    sels[i].element = p->element;
    sels[i].class_  = (char *)p->get_attr("CLASS");
    sels[i].id      = (char *)p->get_attr("ID");
    if (sels[i].element == HD_ELEMENT_A && p->get_attr("HREF") != NULL)
      sels[i].pseudo = (char *)"link";
    else
      sels[i].pseudo = NULL;
  }

  // Do the search...
  return (find_style(i, sels));
}


//
// 'hdStyleSheet::find_style()' - Find the default style for the given
//                                selectors.
//

hdStyle *					// O - Style record
hdStyleSheet::find_style(int        nsels,	// I - Number of selectors
                         hdSelector *sels,	// I - Selectors
			 int        exact)	// I - Exact match required?
{
  int		i, j;				// Looping vars
  hdStyle	*s,				// Current style
		*best;				// Best match
  int		score,				// Current score
		best_score;			// Best match score
  hdElement	e;				// Top-level element


  // Check quickly to see if we have any style info for this element...
  e = sels[0].element;

  if (elements[e] < 0)
    return ((hdStyle *)0);

  // Now loop through the styles for this element to find the best match...
  for (i = elements[e], best = NULL, best_score = 0;
       i < num_styles && styles[i]->selectors[0].element == e;
       i ++)
  {
    s = styles[i];

    if (exact && nsels != s->num_selectors)
      continue;

    for (j = 0, score = 0; j < nsels && j < s->num_selectors; j ++, score <<= 2)
    {
      // Check the element...
      if (sels[j].element != s->selectors[j].element &&
          s->selectors[j].element != HD_ELEMENT_NONE)
      {
        // An element mismatch is an instant no-match...
        score = 0;
	break;
      }

      // Check the class name...
      if ((sels[j].class_ != NULL) == (s->selectors[j].class_ != NULL) &&
          (sels[j].class_ == NULL ||
	   strcasecmp(sels[j].class_, s->selectors[j].class_) == 0))
	score ++;

      // Check the pseudo name...
      if ((sels[j].pseudo != NULL) == (s->selectors[j].pseudo != NULL) &&
          (sels[j].pseudo == NULL ||
	   strcasecmp(sels[j].pseudo, s->selectors[j].pseudo) == 0))
	score ++;

      // Check the id...
      if ((sels[j].id != NULL) == (s->selectors[j].id != NULL) &&
          (sels[j].id == NULL ||
	   strcasecmp(sels[j].id, s->selectors[j].id) == 0))
	score ++;

      if (exact && (score & 3) != 3)
      {
        // No exact match...
        score = 0;
	break;
      }
    }

    // Now update the best match if we get a better score...
    if (score > best_score)
    {
      best_score = score;
      best       = s;
    }
  }

  // Return the best match...
  return (best);
}


//
// 'hdStyleSheet::get_private_style()' - Get a private style definition.
//

hdStyle	*				// O - New style
hdStyleSheet::get_private_style(hdTree *t)
					// I - Tree node that needs style
{
  hdStyle	*parent,		// Parent style
		*style;			// New private style
  hdSelector	selector;		// Selector for private style
  char		id[16];			// Selector ID
  const char	*style_attr;		// STYLE attribute, if any


  // Find the parent style...
  parent = find_style(t);

  // Setup a private selector ID for this node...
  sprintf(id, "_HD_%08X", private_id ++);

  // Create a new style derived from this node...
  selector.set(t->element, NULL, NULL, id);

  style = new hdStyle(1, &selector, t->style);

  style->inherit(parent);

  // Apply the STYLE attribute for this node, if any...
  if ((style_attr = t->get_attr("STYLE")) != NULL)
    style->load(this, style_attr);

  // Add the style to the stylesheet...
  add_style(style);

  // Return the new style...
  return (style);
}


//
// 'hdStyleSheet::load()' - Load a stylesheet from the given file.
//

int					// O - 0 on success, -1 on failure
hdStyleSheet::load(hdFile     *f,	// I - File to read from
                   const char *path)	// I - Search path for included files
{
  int		i, j;			// Looping vars...
  int		status;			// Load status
  int		ch;			// Character from file
  char		filename[1024];		// Include file
  hdFile	*include;		// Include file pointer
  char		sel_s[1024],		// Selector string
		sel_p[256],		// Selector pattern
		*sel_class,		// Selector class
		*sel_pseudo,		// Selector pseudo-target
		*sel_id;		// Selector ID
  int		cur_style,		// Current style
		num_styles,		// Number of styles to create
		num_selectors[HD_SELECTOR_MAX];
					// Number of selectors for each style
  hdSelector	*selectors[HD_SELECTOR_MAX],
  					// Selectors for each style
		parent;			// Parent style
  hdStyle	*style;			// New style
  char		props[4096],		// Style properties
		props_p[256];		// Property pattern


  // Initialize the read patterns.
  pattern("a-zA-Z0-9@.:#", sel_p);
  pattern("~}", props_p);

  // Loop until we can't read any more...
  cur_style  = 0;
  num_styles = 0;
  status     = 0;

  while ((ch = f->get()) != EOF)
  {
    // Skip whitespace...
    if (isspace(ch) || ch == '}')
      continue;

    if (ch == '/')
    {
      // Check for C-style comment...
      if ((ch = f->get()) != '*')
      {
        fprintf(stderr, "Bad sequence \"/%c\" in stylesheet!\n", ch);
	status = -1;
	break;
      }

      // OK, now read chars until EOF or "*/"...
      while ((ch = f->get()) != EOF)
        if (ch == '*')
	{
	  if ((ch = f->get()) == '/')
	    break;
	  else
	    f->unget(ch);
	}

      if (ch != '/')
      {
        fputs("Unterminated comment in stylesheet!\n", stderr);
	status = -1;
	break;
      }

      continue;
    }
    else if (ch == '{')
    {
      // Handle grouping for rendering intent...
      if (num_styles == 0)
        continue;

      // Read property data...
      if (read(f, props_p, props, sizeof(props)) == NULL)
      {
        fputs("Missing property data in stylesheet!\n", stderr);
	status = -1;
	break;
      }

      // Apply properties to all styles...
#ifdef DEBUG
      printf("num_styles = %d\n", num_styles);
#endif // DEBUG

      for (i = 0; i < num_styles; i ++)
      {
        if (selectors[i]->element == HD_ELEMENT_NONE)
	{
	  // Create an instance of this style for each element...
	  for (j = HD_ELEMENT_A; j < HD_ELEMENT_MAX; j ++)
	  {
	    selectors[i]->element = (hdElement)j;
            parent.element        = selectors[i]->element;

            if ((style = find_style(num_selectors[i], selectors[i], 1)) == NULL)
            {
	      style = new hdStyle(num_selectors[i], selectors[i],
	                	  find_style(1, &parent, 1));
              add_style(style);
	    }

            style->load(this, props);
	  }

	  selectors[i]->element = HD_ELEMENT_NONE;
	}
	else
	{
	  // Apply to just the selected element...
          parent.element = selectors[i]->element;

          if ((style = find_style(num_selectors[i], selectors[i], 1)) == NULL)
          {
	    style = new hdStyle(num_selectors[i], selectors[i],
	                	find_style(1, &parent, 1));
            add_style(style);
	  }

          style->load(this, props);
        }

        delete[] selectors[i];
      }

      // Reset to beginning...
      cur_style  = 0;
      num_styles = 0;

      continue;
    }
    else if (ch == ',')
    {
      // Add another selector...
      cur_style ++;

      if (cur_style >= HD_SELECTOR_MAX)
      {
        fprintf(stderr, "Too many selectors (> %d) in stylesheet!\n",
	        HD_SELECTOR_MAX);
	status = -1;
	break;
      }

      continue;
    }
    else if (!sel_p[ch])
    {
      // Not a valid selector string...
      fprintf(stderr, "Bad stylesheet character \"%c\"!\n", ch);
      status = -1;
      break;
    }

    // Read the selector string...
    f->unget(ch);

    read(f, sel_p, sel_s, sizeof(sel_s));

    // OK, got a selector, see if it is @foo...
    if (sel_s[0] == '@')
    {
      // @ selector...
      continue;
    }

    // Allocate memory for the selectors (up to HD_SELECTOR_MAX of them) as needed...
    if (cur_style >= num_styles)
    {
      num_styles ++;
      num_selectors[cur_style] = 0;
      selectors[cur_style]     = new hdSelector[HD_SELECTOR_MAX];
    }

    // Separate the selector string into its components...
    sel_class  = strchr(sel_s, '.');
    sel_pseudo = strchr(sel_s, ':');
    sel_id     = strchr(sel_s, '#');

    if (sel_class)
      *sel_class++ = '\0';

    if (sel_pseudo)
      *sel_pseudo++ = '\0';

    if (sel_id)
      *sel_id++ = '\0';

    if (hdTree::get_element(sel_s) == HD_ELEMENT_UNKNOWN)
      printf("UNKNOWN ELEMENT %s!\n", sel_s);

    // Insert the new selector before any existing ones...
    if (num_selectors[cur_style] > 0)
      memmove(selectors[cur_style] + 1, selectors[cur_style],
              num_selectors[cur_style] * sizeof(hdSelector));

    selectors[cur_style]->set(hdTree::get_element(sel_s),
                              sel_class, sel_pseudo, sel_id);

    num_selectors[cur_style] ++;
  }

  // Clear any selectors still in memory...
  for (i = 0; i < num_styles; i ++)
  {
    for (j = 0; j < num_selectors[i]; j ++)
      selectors[i]->clear();

    delete[] selectors[i];
  }

  // Return the load status...
  return (status);
}


//
// 'hdStyleSheet::pattern()' - Initialize a regex pattern buffer...
//

void
hdStyleSheet::pattern(const char *r,	// I - Regular expression pattern
                      char       p[256])// O - Character lookup table
{
  int	s;				// Set state
  int	ch,				// Char for range
	end;				// Last char in range


  // The regex pattern string "r" can be any regex character pattern,
  // e.g.:
  //
  //    a-zA-Z      Matches all letters
  //    \-+.0-9      Matches all numbers, +, -, and .
  //    ~ \t\n      Matches anything except whitespace.
  //
  // A leading '~' inverts the logic, e.g. all characters *except*
  // those listed.  If you want to match the dash (-) then it must
  // appear be quoted (\-)...

  // Set the logic mode...
  if (*r == '~')
  {
    // Invert logic
    s = 0;
    r ++;
  }
  else
    s = 1;

  // Initialize the pattern buffer...
  memset(p, !s, 256);

  // Loop through the pattern string, updating the pattern buffer as needed.
  for (; *r; r ++)
  {
    if (*r == '\\')
    {
      // Handle quoted char...
      r ++;

      switch (*r)
      {
        case 'n' :
            ch = '\n';
	    break;

        case 'r' :
            ch = '\r';
	    break;

        case 't' :
            ch = '\t';
	    break;

        default :
            ch = *r;
	    break;
      }
    }
    else
      ch = *r;

    // Set this character...
    p[ch] = s;

    // Look ahead to see if we have a range...
    if (r[1] == '-')
    {
      // Yes, grab end character...
      r += 2;

      if (*r == '\\')
      {
	r ++;

	switch (*r)
	{
          case 'n' :
              end =  '\n';
	      break;

          case 'r' :
              end =  '\r';
	      break;

          case 't' :
              end =  '\t';
	      break;

          default :
              end =  *r;
	      break;
	}
      }
      else if (*r)
	end =  *r;
      else
        end =  255;

      // Loop through all chars until we are done...
      for (ch ++; ch <= end; ch ++)
        p[ch] = s;
    }
  }
}


//
// 'hdStyleSheet::read()' - Read a string from the given file.
//

char *					// O - String or NULL on EOF
hdStyleSheet::read(hdFile     *f,	// I - File to read from
                   const char *p,	// I - Allowed chars pattern buffer
		   char       *s,	// O - String buffer
		   int        slen)	// I - Number of bytes in string buffer
{
  int	ch;
  char	*ptr,
	*end;


  // Setup pointers for the start and end of the buffer...
  ptr = s;
  end = s + slen - 1;

  // Loop until we hit EOF or a character that is not allowed...
  while (ptr < end && (ch = f->get()) != EOF)
    if (p[ch])
      *ptr++ = ch;
    else
    {
      f->unget(ch);
      break;
    }

  // Nul-terminate the string...
  *ptr = '\0';

  // Return the string if it is not empty...
  if (ptr > s)
    return (s);
  else
    return (NULL);
}


//
// 'hdStyleSheet::set_charset()' - Set the document character set.
//

void
hdStyleSheet::set_charset(const char *cs)// I - Character set name
{
  int		i;			// Looping var
  char		filename[1024];		// Glyphs filename
  hdFile	*fp;			// Glyphs file
  char		line[256];		// Line from file
  int		code;			// Unicode number
  char		name[255];		// Name string


  // Validate the character set name...
  if (cs == NULL || strchr(cs, '/') != NULL)
  {
    fprintf(stderr, "Bad character set \"%s\"!\n", cs ? cs : "(null)");
    return;
  }

  // Open the charset file...
  snprintf(filename, sizeof(filename), "%s/data/%s.charset",
           hdGlobal.datadir, cs);
  if ((fp = hdFile::open(filename, HD_FILE_READ)) == NULL)
  {
    fprintf(stderr, "Unable to open character set file \"%s\"!\n", filename);
    return;
  }

  // Read the charset type (8bit or unicode)...
  if (fp->gets(line, sizeof(line)) == NULL)
  {
    fprintf(stderr, "Unable to read charset type from \"%s\"!\n", filename);
    delete fp;
    return;
  }

  if (strcasecmp(line, "8bit") != 0 && strcasecmp(line, "unicode") != 0)
  {
    fprintf(stderr, "Bad charset type \"%s\" in \"%s\"!\n", line, filename);
    delete fp;
    return;
  }

  // Free the old charset stuff...
  if (charset)
    free(charset);

  if (num_glyphs)
  {
    for (i = 0; i < num_glyphs; i ++)
      if (glyphs[i])
        free(glyphs[i]);

    delete[] glyphs;
  }

  // Allocate the charset array...
  if (line[0] == '8')
    num_glyphs = 256;
  else
    num_glyphs = 65536;

  charset = strdup(cs);
  glyphs  = new char *[num_glyphs];

  memset(glyphs, 0, num_glyphs * sizeof(char *));

  // Now read all of the remaining lines from the file in the format:
  //
  //     HHHH glyph-name

  while (fp->gets(line, sizeof(line)) != NULL)
  {
    if (sscanf(line, "%x%254s", &code, name) != 2)
    {
      fprintf(stderr, "Bad line \"%s\" in \"%s\"!\n", line, filename);
      break;
    }

    if (code < 0 || code >= num_glyphs)
    {
      fprintf(stderr, "Invalid code %x in \"%s\"!\n", code, filename);
      break;
    }

    glyphs[code] = strdup(name);
  }

  delete fp;
}


//
// 'hdStyleSheet::set_margins()' - Set the page margins.
//

void
hdStyleSheet::set_margins(float l,	// I - Left margin in points
                          float b,	// I - Bottom margin in points
			  float r,	// I - Right margin in points
			  float t)	// I - Top margin in points
{
  left   = l;
  bottom = b;
  right  = r;
  top    = t;

  update_printable();
}


//
// 'hdStyleSheet::set_orientation()' - Set the page orientation.
//

void
hdStyleSheet::set_orientation(hdOrientation o)	// I - Orientation
{
  orientation = o;

  update_printable();
}


//
// 'hdStyleSheet::set_size()' - Set the page size by numbers.
//

void
hdStyleSheet::set_size(float w,		// I - Width in points
                       float l)		// I - Length in points
{
  hdPageSize	*s;			// Current size record


  // Lookup the size in the size table...
  if ((s = hdGlobal.find_size(w, l)) != NULL)
  {
    // Use the standard size name...
    strncpy(size_name, s->name, sizeof(size_name) - 1);
    size_name[sizeof(size_name) - 1] = '\0';
  }
  else
  {
    // If the size wasn't found, use wNNNhNNN...
    sprintf(size_name, "w%dh%d", (int)w, (int)l);
  }

  // Now set the page size and update the printable area...
  width  = w;
  length = l;

  update_printable();
}


//
// 'hdStyleSheet::set_size()' - Set the page size by name.
//

void
hdStyleSheet::set_size(const char *name)// I - Page size name
{
  hdPageSize	*s;			// Current size record
  int		w, l;			// Width and length in points


  // Lookup the size in the size table...
  if ((s = hdGlobal.find_size(name)) != NULL)
  {
    // Use the standard size...
    strncpy(size_name, s->name, sizeof(size_name) - 1);
    size_name[sizeof(size_name) - 1] = '\0';

    width  = s->width;
    length = s->length;

    update_printable();
  }
  else
  {
    // OK, that didn't work; see if the name is of the form "wNNNhNNN"...
    if (sscanf(name, "w%dh%d", &w, &l) == 2)
    {
      // Yes, it is a custom page size; set it...
      strncpy(size_name, name, sizeof(size_name) - 1);
      size_name[sizeof(size_name) - 1] = '\0';

      width  = w;
      length = l;

      update_printable();
    }
  }
}


//
// 'hdStyleSheet::update_printable()' - Update the printable page area.
//

void
hdStyleSheet::update_printable()
{
  switch (orientation)
  {
    case HD_ORIENTATION_PORTRAIT :
    case HD_ORIENTATION_REVERSE_PORTRAIT :
        print_width  = width - left - right;
	print_length = length - top - bottom;
	break;

    case HD_ORIENTATION_LANDSCAPE :
    case HD_ORIENTATION_REVERSE_LANDSCAPE :
	print_width  = length - left - right;
        print_length = width - top - bottom;
	break;
  }
}


//
// 'hdStyleSheet::update_styles()' - Update all relative style data.
//

void
hdStyleSheet::update_styles()
{
  int		i;		// Looping var
  hdStyle	**style;	// Current style


  // First clear the "updated" state of all styles...
  for (i = num_styles, style = styles; i > 0; i --, style ++)
    (*style)->updated = 0;

  // Then update all the styles...
  for (i = num_styles, style = styles; i > 0; i --, style ++)
    (*style)->update(this);
}


//
// End of "$Id: stylesheet.cxx,v 1.3 2002/02/08 19:39:51 mike Exp $".
//