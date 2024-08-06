/********************************************************************\

  Name:         msequencer.cxx
  Created by:   Stefan Ritt

  Contents:     MIDAS sequencer engine

\********************************************************************/

#include "midas.h"
#include "msystem.h"
#include "mvodb.h"
#include "mxml.h"
#include "sequencer.h"
#include "strlcpy.h"
#include "tinyexpr.h"
#include "odbxx.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <vector>

#define XNAME_LENGTH 256

/**dox***************************************************************/
/** @file sequencer.cxx
The Midas Sequencer file
*/

/** @defgroup seqfunctioncode Sequencer Functions
 */

/**dox***************************************************************/
/** @addtogroup seqfunctioncode
 *
 *  @{  */

/*------------------------------------------------------------------*/

SEQUENCER_STR(sequencer_str);

MVOdb *gOdb = NULL;

// sequencer context

class SeqCon
{
public:
   PMXML_NODE pnseq = NULL;
   MVOdb *odbs = NULL;
   SEQUENCER *seqp = NULL;
   HNDLE hDB  = 0;
   HNDLE hSeq = 0;
   std::string seq_name;
   std::string prg_name;
   std::string odb_path;
};

/*------------------------------------------------------------------*/

char *stristr(const char *str, const char *pattern) {
   char c1, c2, *ps, *pp;

   if (str == NULL || pattern == NULL)
      return NULL;

   while (*str) {
      ps = (char *) str;
      pp = (char *) pattern;
      c1 = *ps;
      c2 = *pp;
      if (toupper(c1) == toupper(c2)) {
         while (*pp) {
            c1 = *ps;
            c2 = *pp;

            if (toupper(c1) != toupper(c2))
               break;

            ps++;
            pp++;
         }

         if (!*pp)
            return (char *) str;
      }
      str++;
   }

   return NULL;
}

/*------------------------------------------------------------------*/

void strsubst(char *string, int size, const char *pattern, const char *subst)
/* subsitute "pattern" with "subst" in "string" */
{
   char *tail, *p;
   int s;

   p = string;
   for (p = stristr(p, pattern); p != NULL; p = stristr(p, pattern)) {

      if (strlen(pattern) == strlen(subst)) {
         memcpy(p, subst, strlen(subst));
      } else if (strlen(pattern) > strlen(subst)) {
         memcpy(p, subst, strlen(subst));
         memmove(p + strlen(subst), p + strlen(pattern), strlen(p + strlen(pattern)) + 1);
      } else {
         tail = (char *) malloc(strlen(p) - strlen(pattern) + 1);
         strcpy(tail, p + strlen(pattern));
         s = size - (p - string);
         strlcpy(p, subst, s);
         strlcat(p, tail, s);
         free(tail);
         tail = NULL;
      }

      p += strlen(subst);
   }
}

/*------------------------------------------------------------------*/

static std::string toString(int v) {
   char buf[256];
   sprintf(buf, "%d", v);
   return buf;
}

static std::string qtoString(std::string filename, int line, int level) {
   std::string buf = "l=\"" + std::to_string(line) + "\"";
   buf += " fn=\"" + filename + "\"";
   if (level)
      buf += " lvl=\"" + std::to_string(level) + "\"";
   return buf;
}

static std::string q(const char *s) {
   return "\"" + std::string(s) + "\"";
}

/*------------------------------------------------------------------*/

bool is_valid_number(const char *str) {
   std::string s(str);
   std::stringstream ss;
   ss << s;
   double num = 0;
   ss >> num;
   if (ss.good())
      return false;
   else if (num == 0 && s[0] != '0')
      return false;
   else if (s[0] == 0)
      return false;
   return true;
}

/*------------------------------------------------------------------*/

void seq_error(SEQUENCER &seq, SeqCon* c, const char *str) {
   int status;
   HNDLE hKey;

   strlcpy(seq.error, str, sizeof(seq.error));
   seq.error_line = seq.current_line_number;
   seq.serror_line = seq.scurrent_line_number;
   seq.running = FALSE;
   seq.transition_request = FALSE;

   status = db_find_key(c->hDB, c->hSeq, "State", &hKey);
   if (status != DB_SUCCESS)
      return;
   status = db_set_record(c->hDB, hKey, &seq, sizeof(seq), 0);
   if (status != DB_SUCCESS)
      return;

   cm_msg(MTALK, "sequencer", "Sequencer has stopped with error.");
}

/*------------------------------------------------------------------*/

int strbreak(char *str, char list[][XNAME_LENGTH], int size, const char *brk, BOOL ignore_quotes)
/* break comma-separated list into char array, stripping leading
 and trailing blanks */
{
   int i, j;
   char *p;

   memset(list, 0, size * XNAME_LENGTH);
   p = str;
   if (!p || !*p)
      return 0;

   while (*p == ' ')
      p++;

   for (i = 0; *p && i < size; i++) {
      if (*p == '"' && !ignore_quotes) {
         p++;
         j = 0;
         memset(list[i], 0, XNAME_LENGTH);
         do {
            /* convert two '"' to one */
            if (*p == '"' && *(p + 1) == '"') {
               list[i][j++] = '"';
               p += 2;
            } else if (*p == '"') {
               break;
            } else
               list[i][j++] = *p++;

         } while (j < XNAME_LENGTH - 1);
         list[i][j] = 0;

         /* skip second '"' */
         p++;

         /* skip blanks and break character */
         while (*p == ' ')
            p++;
         if (*p && strchr(brk, *p))
            p++;
         while (*p == ' ')
            p++;

      } else {
         strlcpy(list[i], p, XNAME_LENGTH);

         for (j = 0; j < (int) strlen(list[i]); j++)
            if (strchr(brk, list[i][j])) {
               list[i][j] = 0;
               break;
            }

         p += strlen(list[i]);
         while (*p == ' ')
            p++;
         if (*p && strchr(brk, *p))
            p++;
         while (*p == ' ')
            p++;

         while (list[i][strlen(list[i]) - 1] == ' ')
            list[i][strlen(list[i]) - 1] = 0;
      }

      if (!*p)
         break;
   }

   if (i == size)
      return size;

   return i + 1;
}

/*------------------------------------------------------------------*/

extern char *stristr(const char *str, const char *pattern);

/*------------------------------------------------------------------*/

std::string eval_var(SEQUENCER &seq, SeqCon* c, std::string value) {
   std::string result;
   std::string xpath;
   xpath += "/";
   xpath += c->odb_path;

   //printf("eval [%s] xpath [%s]\n", value.c_str(), xpath.c_str());

   result = value;

   // replace all $... with value
   int i1, i2;
   std::string vsubst;
   while ((i1 = (int)result.find("$")) != (int)std::string::npos) {
      std::string s = result.substr(i1 + 1);
      if (std::isdigit(s[0])) {
         // find end of number
         for (i2 = i1 + 1; std::isdigit(result[i2]);)
            i2++;

         // replace all $<number> with subroutine parameters
         int index = atoi(s.c_str());
         if (seq.stack_index > 0) {
            std::istringstream f(seq.subroutine_param[seq.stack_index - 1]);
            std::vector<std::string> param;
            std::string sp;
            while (std::getline(f, sp, ','))
               param.push_back(sp);
            if (index == 0 || index > (int)param.size())
               throw "Parameter \"$" + std::to_string(index) + "\" not valid";
            vsubst = param[index - 1];
            if (vsubst[0] == '$')
               vsubst = eval_var(seq, c, vsubst);
         } else
            throw "Parameter \"$" + std::to_string(index) + "\" not valid";
      } else {
         // find end of string
         for (i2 = i1 + 1; std::isalnum(result[i2]) || result[i2] == '_';)
            i2++;
         s = s.substr(0, i2 - i1 - 1);
         if (result[i2] == '[') {
            // array
            auto sindex = result.substr(i2+1);
            int i = sindex.find(']');
            if (i == (int)std::string::npos)
               throw "Variable \"" + result +"\" does not contain ']'";
            sindex = sindex.substr(0, i);
            sindex = eval_var(seq, c, sindex);
            int index;
            try {
               index = std::stoi(sindex);
            } catch (...) {
               throw "Variable \"" + s + "\" has invalid index";
            }

            try {
               midas::odb o(xpath + "/Variables/" + s);
               std::vector<std::string> sv = o;
               vsubst = sv[index];
            } catch (...) {
               throw "Variable \"" + s + "\" not found";
            }
            while (result[i2] && result[i2] != ']')
               i2++;
            if (!result[i2])
               throw "Variable \"" + result +"\" does not contain ']'";
            if (result[i2] == ']')
               i2++;
         } else {
            try {
               midas::odb o(xpath + "/Variables/" + s);
               vsubst = o;
            } catch (...) {
               throw "Variable \"" + s + "\" not found";
            }
         }
      }

      result = result.substr(0, i1) + vsubst + result.substr(i2);
   }

   //printf("eval [%s] xpath [%s] result [%s]\n", value.c_str(), xpath.c_str(), result.c_str());

   // check if result is a list
   if (result.find(",") != std::string::npos)
      return result;

   // check for expression
   int error;
   double r = te_interp(result.c_str(), &error);
   if (error > 0) {
      // check if result is only a string
      if (!std::isdigit(result[0]) && result[0] != '-')
         return result;

      throw "Error in expression \"" + result + "\" position " + std::to_string(error - 1);
   }

   if (r == (int) r)
      return std::to_string((int) r);

   return std::to_string(r);
}

/*------------------------------------------------------------------*/

int concatenate(SEQUENCER &seq, SeqCon* c, char *result, int size, char *value) {
   char list[100][XNAME_LENGTH];
   int i, n;

   n = strbreak(value, list, 100, ",", FALSE);

   result[0] = 0;
   for (i = 0; i < n; i++) {
      std::string str = eval_var(seq, c, std::string(list[i]));
      strlcat(result, str.c_str(), size);
   }

   return TRUE;
}

/*------------------------------------------------------------------*/

int eval_condition(SEQUENCER &seq, SeqCon* c, const char *condition) {
   int i;
   double value1, value2;
   char value1_str[256], value2_str[256], str[256], op[3], *p;
   std::string value1_var, value2_var;

   // strip leading and trailing space
   p = (char *)condition;
   while (*p == ' ')
      p++;
   strcpy(str, p);

   // strip any comment '#'
   if (strchr(str, '#'))
      *strchr(str, '#') = 0;

   while (strlen(str) > 0 && (str[strlen(str)-1] == ' '))
      str[strlen(str)-1] = 0;

   // strip enclosing '()'
   if (str[0] == '(' && strlen(str) > 0 && str[strlen(str)-1] == ')') {
      strlcpy(value1_str, str+1, sizeof(value1_str));
      strlcpy(str, value1_str, sizeof(str));
      str[strlen(str)-1] = 0;
   }

   op[1] = op[2] = 0;

   /* find value and operator */
   for (i = 0; i < (int) strlen(str); i++)
      if (strchr("<>=!&", str[i]) != NULL)
         break;
   strlcpy(value1_str, str, i + 1);
   while (value1_str[strlen(value1_str) - 1] == ' ')
      value1_str[strlen(value1_str) - 1] = 0;
   op[0] = str[i];
   if (strchr("<>=!&", str[i + 1]) != NULL)
      op[1] = str[++i];

   for (i++; str[i] == ' '; i++);
   strlcpy(value2_str, str + i, sizeof(value2_str));

   value1_var = eval_var(seq, c, value1_str);
   value2_var = eval_var(seq, c, value2_str);

   if (!is_valid_number(value1_var.c_str()) || !is_valid_number(value2_var.c_str())) {
      // string comparison
      if (strcmp(op, "=") == 0)
         return equal_ustring(value1_var.c_str(), value2_var.c_str()) ? 1 : 0;
      if (strcmp(op, "==") == 0)
         return equal_ustring(value1_var.c_str(), value2_var.c_str()) ? 1 : 0;
      if (strcmp(op, "!=") == 0)
         return equal_ustring(value1_var.c_str(), value2_var.c_str()) ? 0 : 1;
      // invalid operator for string comparisons
      return -1;
   }

   // numeric comparison
   value1 = atof(value1_var.c_str());
   value2 = atof(value2_var.c_str());

   /* now do logical operation */
   if (strcmp(op, "=") == 0)
      if (value1 == value2)
         return 1;
   if (strcmp(op, "==") == 0)
      if (value1 == value2)
         return 1;
   if (strcmp(op, "!=") == 0)
      if (value1 != value2)
         return 1;
   if (strcmp(op, "<") == 0)
      if (value1 < value2)
         return 1;
   if (strcmp(op, ">") == 0)
      if (value1 > value2)
         return 1;
   if (strcmp(op, "<=") == 0)
      if (value1 <= value2)
         return 1;
   if (strcmp(op, ">=") == 0)
      if (value1 >= value2)
         return 1;
   if (strcmp(op, "&") == 0)
      if (((unsigned int) value1 & (unsigned int) value2) > 0)
         return 1;

   return 0;
}

/*------------------------------------------------------------------*/

bool loadMSL(MVOdb *o, const std::string &filename) {

   std::ifstream file(filename);
   if (!file.is_open())
      return false;

   std::vector<std::string> lines;
   std::string line;

   // read all lines from file and put it into array
   while (std::getline(file, line))
      lines.push_back(line);
   file.close();

   o->WSA("Script/Lines", lines, 0);

   return true;
}

/*------------------------------------------------------------------*/

static BOOL msl_parse(SEQUENCER& seq, SeqCon*c, int level, const char *cpath, const char *filename, const char *xml_filename,
                      char *error, int error_size, int *error_line) {
   char str[256], *buf, *pl, *pe;
   char list[100][XNAME_LENGTH], list2[100][XNAME_LENGTH], **lines;
   int i, j, n, nq, size, n_lines, endl, line, nest, incl, library;
   std::string xml, path, fn;
   char *msl_include, *xml_include, *include_error;
   int include_error_size;
   BOOL include_status, rel_path;

   if (level > 10) {
      sprintf(error, "More than 10 nested INCLUDE statements exceeded in file \"%s\"", filename);
      return FALSE;
   }

   rel_path = (filename[0] != DIR_SEPARATOR);
   path = std::string(cpath);
   if (path.length() > 0 && path.back() != '/')
      path += '/';

   std::string fullFilename;
   if (rel_path)
      fullFilename = path + filename;
   else
      fullFilename = filename;
   int fhin = open(fullFilename.c_str(), O_RDONLY | O_TEXT);
   if (fhin < 0) {
      sprintf(error, "Cannot open \"%s\", errno %d (%s)", fullFilename.c_str(), errno, strerror(errno));
      return FALSE;
   }
   std::string s;
   if (rel_path)
      s = path + xml_filename;
   else
      s = xml_filename;

   FILE *fout = fopen(s.c_str(), "wt");
   if (fout == NULL) {
      sprintf(error, "Cannot write to \"%s\", fopen() errno %d (%s)", s.c_str(), errno, strerror(errno));
      return FALSE;
   }

   size = (int) lseek(fhin, 0, SEEK_END);
   lseek(fhin, 0, SEEK_SET);
   buf = (char *) malloc(size + 1);
   size = (int) read(fhin, buf, size);
   buf[size] = 0;
   close(fhin);

   /* look for any includes */
   lines = (char **) malloc(sizeof(char *));
   incl = 0;
   pl = buf;
   library = FALSE;
   for (n_lines = 0; *pl; n_lines++) {
      lines = (char **) realloc(lines, sizeof(char *) * (n_lines + 1));
      lines[n_lines] = pl;
      if (strchr(pl, '\n')) {
         pe = strchr(pl, '\n');
         *pe = 0;
         if (*(pe - 1) == '\r') {
            *(pe - 1) = 0;
         }
         pe++;
      } else
         pe = pl + strlen(pl);
      strlcpy(str, pl, sizeof(str));
      pl = pe;
      strbreak(str, list, 100, ", ", FALSE);
      if (equal_ustring(list[0], "include")) {
         if (!incl) {
            xml += "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n";
            xml += "<!DOCTYPE RunSequence [\n";
            incl = 1;
         }

         // if a path is given, use filename as entity reference
         char *reference = strrchr(list[1], '/');
         if (reference)
            reference++;
         else
            reference = list[1];

         std::string p;
         if (list[1][0] == '/') {
            p = std::string(cm_get_path());
            p += "userfiles/sequencer";
            p += list[1];
         } else {
            p = path;
            if (!p.empty() && p.back() != '/')
               p += "/";
            p += list[1];
         }

         xml += "  <!ENTITY ";
         xml += reference;
         xml += " SYSTEM \"";
         xml += p;
         xml += ".xml\">\n";

         // recurse
         size = p.length() + 1 + 4;
         msl_include = (char *) malloc(size);
         xml_include = (char *) malloc(size);
         strlcpy(msl_include, p.c_str(), size);
         strlcpy(xml_include, p.c_str(), size);
         strlcat(msl_include, ".msl", size);
         strlcat(xml_include, ".xml", size);

         include_error = error + strlen(error);
         include_error_size = error_size - strlen(error);

         std::string path(cm_get_path());
         path += "userfiles/sequencer/";
         path += seq.path;

         include_status = msl_parse(seq, c, level+1, path.c_str(), msl_include, xml_include, include_error, include_error_size, error_line);
         free(msl_include);
         free(xml_include);

         if (!include_status) {
            // report the error on CALL line instead of the one in included file
            *error_line = n_lines + 1;
            return FALSE;
         }
      }
      if (equal_ustring(list[0], "library")) {
         xml += "<Library name=\"";
         xml += list[1];
         xml += "\">\n";
         library = TRUE;
      }
   }
   if (incl) {
      xml += "]>\n";
   } else if (!library) {
      xml += "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n";
   }

   /* parse rest of file */
   if (!library) {
      xml += "<RunSequence xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"\">\n";
   }

   std::vector<std::string> slines;
   for (line = 0; line < n_lines; line++) {
      slines.push_back(lines[line]);
   }

   c->odbs->WSA("Script/Lines", slines, 0);

   /* clear all variables */

   if (!seq.running) {
      c->odbs->Delete("Variables");
      c->odbs->Delete("Param");
   }

   for (line = 0; line < n_lines; line++) {
      char *p = lines[line];
      while (*p == ' ')
         p++;
      strlcpy(list[0], p, sizeof(list[0]));
      if (strchr(list[0], ' '))
         *strchr(list[0], ' ') = 0;
      p += strlen(list[0]);
      n = strbreak(p + 1, &list[1], 99, ",", FALSE) + 1;

      /* remove any comment */
      for (i = 0; i < n; i++) {
         if (list[i][0] == '#') {
            for (j = i; j < n; j++)
               list[j][0] = 0;
            break;
         }
      }

      /* check for variable assignment */
      char eq[1024];
      strlcpy(eq, lines[line], sizeof(eq));
      if (strchr(eq, '#'))
         *strchr(eq, '#') = 0;
      for (i = 0, n = 0, nq = 0; i < (int)strlen(eq); i++) {
         if (eq[i] == '\"')
            nq = (nq == 0 ? 1 : 0);
         if (eq[i] == '=' && nq == 0 &&
             (i > 0 && (eq[i - 1] != '!') && eq[i - 1] != '<' && eq[i - 1] != '>'))
            n++;
      }
      if (n == 1 && eq[0] != '=') {
         // equation found
         strlcpy(list[0], "SET", sizeof(list[0]));
         p = eq;
         while (*p == ' ')
            p++;
         strlcpy(list[1], p, sizeof(list[1]));
         *strchr(list[1], '=') = 0;
         if (strchr(list[1], ' '))
            *strchr(list[1], ' ') = 0;
         p = strchr(eq, '=')+1;
         while (*p == ' ')
            p++;
         strlcpy(list[2], p, sizeof(list[2]));
         while (strlen(list[2]) > 0 && list[2][strlen(list[2])-1] == ' ')
            list[2][strlen(list[2])-1] = 0;
      }

      if (equal_ustring(list[0], "library")) {

      } else if (equal_ustring(list[0], "include")) {
         // if a path is given, use filename as entity reference
         char *reference = strrchr(list[1], '/');
         if (reference)
            reference++;
         else
            reference = list[1];

         xml += "&";
         xml += reference;
         xml += ";\n";

      } else if (equal_ustring(list[0], "call")) {
         xml += "<Call " + qtoString(fullFilename, line + 1, level) + " name=" + q(list[1]) + ">";
         for (i = 2; i < 100 && list[i][0]; i++) {
            if (i > 2) {
               xml += ",";
            }
            xml += list[i];
         }
         xml += "</Call>\n";

      } else if (equal_ustring(list[0], "cat")) {
         xml += "<Cat " + qtoString(fullFilename, line + 1, level) + " name=" + q(list[1]) + ">";
         for (i = 2; i < 100 && list[i][0]; i++) {
            if (i > 2) {
               xml += ",";
            }
            xml += q(list[i]);
         }
         xml += "</Cat>\n";

      } else if (equal_ustring(list[0], "comment")) {
         xml += "<Comment " + qtoString(fullFilename, line + 1, level) + ">" + list[1] + "</Comment>\n";

      } else if (equal_ustring(list[0], "exit")) {
         xml += "<Exit " + qtoString(fullFilename, line + 1, level) + " />\n";

      } else if (equal_ustring(list[0], "goto")) {
         xml += "<Goto " + qtoString(fullFilename, line + 1, level) + " sline=" + q(list[1]) + " />\n";

      } else if (equal_ustring(list[0], "if")) {
         xml += "<If " + qtoString(fullFilename, line + 1, level) + " condition=\"";
         for (i = 1; i < 100 && list[i][0] && stricmp(list[i], "THEN") != 0; i++) {
            xml += list[i];
         }
         xml += "\">\n";

      } else if (equal_ustring(list[0], "else")) {
         xml += "<Else />\n";

      } else if (equal_ustring(list[0], "endif")) {
         xml += "</If>\n";

      } else if (equal_ustring(list[0], "loop")) {
         /* find end of loop */
         for (i = line, nest = 0; i < n_lines; i++) {
            strbreak(lines[i], list2, 100, ", ", FALSE);
            if (equal_ustring(list2[0], "loop"))
               nest++;
            if (equal_ustring(list2[0], "endloop")) {
               nest--;
               if (nest == 0)
                  break;
            }
         }
         if (i < n_lines)
            endl = i + 1;
         else
            endl = line + 1;
         if (list[2][0] == 0) {
            xml += "<Loop " + qtoString(fullFilename, line + 1, level) + " le=\"" + std::to_string(endl) + "\" n=" + q(list[1]) + ">\n";
         } else if (list[3][0] == 0) {
            xml += "<Loop " + qtoString(fullFilename, line + 1, level) + " le=\"" + std::to_string(endl) + "\" var=" + q(list[1]) + " n=" +
                   q(list[2]) + ">\n";
         } else {
            xml += "<Loop " + qtoString(fullFilename, line + 1, level) + " le=\"" + std::to_string(endl) + "\" var=" + q(list[1]) + " values=\"";
            for (i = 2; i < 100 && list[i][0]; i++) {
               if (i > 2) {
                  xml += ",";
               }
               xml += list[i];
            }
            xml += "\">\n";
         }
      } else if (equal_ustring(list[0], "endloop")) {
         xml += "</Loop>\n";

      } else if (equal_ustring(list[0], "break")) {
         xml += "<Break "+ qtoString(fullFilename, line + 1, level) +"></Break>\n";

      } else if (equal_ustring(list[0], "message")) {
         xml += "<Message " + qtoString(fullFilename, line + 1, level);
         if (list[2][0] == '1')
            xml += " wait=\"1\"";
         xml += ">";
         xml += list[1];
         xml += "</Message>\n";

      } else if (equal_ustring(list[0], "msg")) {
         if (list[2][0]) {
            xml += "<Msg " + qtoString(fullFilename, line + 1, level) + " type=\""+list[2]+"\">" + list[1] + "</Msg>\n";
         } else {
            xml += "<Msg " + qtoString(fullFilename, line + 1, level) + ">" + list[1] + "</Msg>\n";
         }

      } else if (equal_ustring(list[0], "odbinc")) {
         if (list[2][0] == 0)
            strlcpy(list[2], "1", 2);
         xml += "<ODBInc " + qtoString(fullFilename, line + 1, level) + " path=" + q(list[1]) + ">" + list[2] + "</ODBInc>\n";

      } else if (equal_ustring(list[0], "odbcreate")) {
         if (list[3][0]) {
            xml += "<ODBCreate " + qtoString(fullFilename, line + 1, level) + " size=" + q(list[3]) + " path=" + q(list[1]) + " type=" +
                   q(list[2]) + "></ODBCreate>\n";
         } else {
            xml += "<ODBCreate " + qtoString(fullFilename, line + 1, level) + " path=" + q(list[1]) + " type=" + q(list[2]) +
                   "></ODBCreate>\n";
         }

      } else if (equal_ustring(list[0], "odbdelete")) {
         xml += "<ODBDelete " + qtoString(fullFilename, line + 1, level) + ">" + list[1] + "</ODBDelete>\n";

      } else if (equal_ustring(list[0], "odbset")) {
         if (list[3][0]) {
            xml += "<ODBSet " + qtoString(fullFilename, line + 1, level) + " notify=" + q(list[3]) + " path=" + q(list[1]) + ">" +
                   list[2] + "</ODBSet>\n";
         } else {
            xml += "<ODBSet " + qtoString(fullFilename, line + 1, level) + " path=" + q(list[1]) + ">" + list[2] + "</ODBSet>\n";
         }

      } else if (equal_ustring(list[0], "odbload")) {
         if (list[2][0]) {
            xml += "<ODBLoad " + qtoString(fullFilename, line + 1, level) + " path=" + q(list[2]) + ">" + list[1] + "</ODBLoad>\n";
         } else {
            xml += "<ODBLoad " + qtoString(fullFilename, line + 1, level) + ">" + list[1] + "</ODBLoad>\n";
         }

      } else if (equal_ustring(list[0], "odbget")) {
         xml += "<ODBGet " + qtoString(fullFilename, line + 1, level) + " path=" + q(list[1]) + ">" + list[2] + "</ODBGet>\n";

      } else if (equal_ustring(list[0], "odbsave")) {
         xml += "<ODBSave " + qtoString(fullFilename, line + 1, level) + " path=" + q(list[1]) + ">" + list[2] + "</ODBSave>\n";

      } else if (equal_ustring(list[0], "odbsubdir")) {
         if (list[2][0]) {
            xml += "<ODBSubdir " + qtoString(fullFilename, line + 1, level) + " notify=" + q(list[2]) + " path=" + q(list[1]) + ">\n";
         } else {
            xml += "<ODBSubdir " + qtoString(fullFilename, line + 1, level) + " path=" + q(list[1]) + ">\n";
         }
      } else if (equal_ustring(list[0], "endodbsubdir")) {
         xml += "</ODBSubdir>\n";

      } else if (equal_ustring(list[0], "param")) {
         if (list[2][0] == 0 || equal_ustring(list[2], "bool")) { // only name, only name and bool
            sprintf(error, "Paremeter \"%s\" misses 'comment'", list[1]);
            *error_line = line + 1;
            return FALSE;

         } else if (!list[4][0] && equal_ustring(list[3], "bool")) { // name, comment and bool
            xml += "<Param " + qtoString(fullFilename, line + 1, level) + " name=" + q(list[1]) + " type=\"bool\" " + " comment=" + q(list[2]) + "/>\n";
            bool v = false;
            c->odbs->RB((std::string("Param/Value/") + list[1]).c_str(), &v, true);
            std::string s;
            c->odbs->WS((std::string("Param/Comment/") + list[1]).c_str(), list[2]);
         } else if (!list[5][0] && equal_ustring(list[4], "bool")) { // name, comment, default and bool
            xml += "<Param " + qtoString(fullFilename, line + 1, level) + " name=" + q(list[1]) + " type=\"bool\" default=" + q(list[4]) +"/>\n";
            bool v = false;
            c->odbs->RB((std::string("Param/Value/") + list[1]).c_str(), &v, true);
            std::string s;
            c->odbs->WS((std::string("Param/Comment/") + list[1]).c_str(), list[2]);
            c->odbs->WS((std::string("Param/Defaults/") + list[1]).c_str(), list[3]);

         } else if (!list[3][0]) { // name and comment
            xml += "<Param " + qtoString(fullFilename, line + 1, level) + " name=" + q(list[1]) + " comment=" + q(list[2]) + " />\n";
            std::string v;
            c->odbs->RS((std::string("Param/Value/") + list[1]).c_str(), &v, true);
            c->odbs->WS((std::string("Param/Comment/") + list[1]).c_str(), list[2]);
         } else if (!list[4][0]) { // name, comment and default
            xml += "<Param " + qtoString(fullFilename, line + 1, level) + " name=" + q(list[1]) + " comment=" + q(list[2]) + " default=" + q(list[3]) + " />\n";
            std::string v;
            c->odbs->RS((std::string("Param/Value/") + list[1]).c_str(), &v, true);
            c->odbs->WS((std::string("Param/Comment/") + list[1]).c_str(), list[2]);
            c->odbs->WS((std::string("Param/Defaults/") + list[1]).c_str(), list[3]);
         } else {
            xml += "<Param " + qtoString(fullFilename, line + 1, level) + " name=" + q(list[1]) + " comment=" + q(list[2]) +
                   " options=\"";
            std::string v;
            c->odbs->RS((std::string("Param/Value/") + list[1]).c_str(), &v, true);
            c->odbs->WS((std::string("Param/Comment/") + list[1]).c_str(), list[2]);
            std::vector<std::string> options;
            for (i = 3; i < 100 && list[i][0]; i++) {
               if (i > 3) {
                  xml += ",";
               }
               xml += list[i];
               options.push_back(list[i]);
            }
            xml += "\" />\n";
            c->odbs->WSA((std::string("Param/Options/") + list[1]).c_str(), options, 0);
         }

      } else if (equal_ustring(list[0], "rundescription")) {
         xml += "<RunDescription " + qtoString(fullFilename, line + 1, level) + ">" + list[1] + "</RunDescription>\n";

      } else if (equal_ustring(list[0], "script")) {
         if (list[2][0] == 0) {
            xml += "<Script " + qtoString(fullFilename, line + 1, level) + ">" + list[1] + "</Script>\n";
         } else {
            xml += "<Script " + qtoString(fullFilename, line + 1, level) + " params=\"";
            for (i = 2; i < 100 && list[i][0]; i++) {
               if (i > 2) {
                  xml += ",";
               }
               xml += list[i];
            }
            xml += "\">";
            xml += list[1];
            xml += "</Script>\n";
         }

      } else if (equal_ustring(list[0], "set")) {
         xml += "<Set " + qtoString(fullFilename, line + 1, level) + " name=" + q(list[1]) + ">" + list[2] + "</Set>\n";

      } else if (equal_ustring(list[0], "subroutine")) {
         xml += "\n<Subroutine " + qtoString(fullFilename, line + 1, level) + " name=" + q(list[1]) + ">\n";

      } else if (equal_ustring(list[0], "endsubroutine")) {
         xml += "</Subroutine>\n";

      } else if (equal_ustring(list[0], "transition")) {
         xml += "<Transition " + qtoString(fullFilename, line + 1, level) + ">" + list[1] + "</Transition>\n";

      } else if (equal_ustring(list[0], "wait")) {
         if (!list[2][0]) {
            xml += "<Wait " + qtoString(fullFilename, line + 1, level) + " for=\"seconds\">" + list[1] + "</Wait>\n";
         } else if (!list[3][0]) {
            xml += "<Wait " + qtoString(fullFilename, line + 1, level) + " for=" + q(list[1]) + ">" + list[2] + "</Wait>\n";
         } else {
            xml += "<Wait " + qtoString(fullFilename, line + 1, level) + " for=" + q(list[1]) + " path=" + q(list[2]) + " op=" +
                   q(list[3]) + ">" + list[4] + "</Wait>\n";
         }

      } else if (list[0][0] == 0 || list[0][0] == '#') {
         /* skip empty or out-commented lines */
      } else {
         sprintf(error, "Invalid command \"%s\"", list[0]);
         *error_line = line + 1;
         return FALSE;
      }
   }

   free(lines);
   free(buf);
   if (library) {
      xml += "\n</Library>\n";
   } else {
      xml += "</RunSequence>\n";
   }

   // write XML to .xml file
   fprintf(fout, "%s", xml.c_str());
   fclose(fout);

   return TRUE;
}

/*------------------------------------------------------------------*/

void seq_read(SEQUENCER *seq, SeqCon* c) {
   int status;
   HNDLE hKey;

   status = db_find_key(c->hDB, c->hSeq, "State", &hKey);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "seq_read", "Cannot find /Sequencer/State in ODB, db_find_key() status %d", status);
      return;
   }

   int size = sizeof(SEQUENCER);
   status = db_get_record1(c->hDB, hKey, seq, &size, 0, strcomb1(sequencer_str).c_str());
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "seq_read", "Cannot get /Sequencer/State from ODB, db_get_record1() status %d", status);
      return;
   }
}

void seq_write(const SEQUENCER &seq, SeqCon* c) {
   int status;
   HNDLE hKey;

   status = db_find_key(c->hDB, c->hSeq, "State", &hKey);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "seq_write", "Cannot find /Sequencer/State in ODB, db_find_key() status %d", status);
      return;
   }
   status = db_set_record(c->hDB, hKey, (void *) &seq, sizeof(SEQUENCER), 0);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "seq_write", "Cannot write to ODB /Sequencer/State, db_set_record() status %d", status);
      return;
   }
}

/*------------------------------------------------------------------*/

void seq_clear(SEQUENCER &seq) {
   seq.running = FALSE;
   seq.finished = FALSE;
   seq.paused = FALSE;
   seq.transition_request = FALSE;
   seq.wait_limit = 0;
   seq.wait_value = 0;
   seq.start_time = 0;
   seq.wait_type[0] = 0;
   seq.wait_odb[0] = 0;
   for (int i = 0; i < SEQ_NEST_LEVEL_LOOP; i++) {
      seq.loop_start_line[i] = 0;
      seq.sloop_start_line[i] = 0;
      seq.loop_end_line[i] = 0;
      seq.sloop_end_line[i] = 0;
      seq.loop_counter[i] = 0;
      seq.loop_n[i] = 0;
   }
   for (int i = 0; i < SEQ_NEST_LEVEL_IF; i++) {
      seq.if_else_line[i] = 0;
      seq.if_endif_line[i] = 0;
   }
   for (int i = 0; i < SEQ_NEST_LEVEL_SUB; i++) {
      seq.subroutine_end_line[i] = 0;
      seq.subroutine_return_line[i] = 0;
      seq.subroutine_call_line[i] = 0;
      seq.ssubroutine_call_line[i] = 0;
      seq.subroutine_param[i][0] = 0;
   }
   seq.current_line_number = 0;
   seq.scurrent_line_number = 0;
   seq.sfilename[0] = 0;
   seq.if_index = 0;
   seq.stack_index = 0;
   seq.error[0] = 0;
   seq.error_line = 0;
   seq.serror_line = 0;
   seq.subdir[0] = 0;
   seq.subdir_end_line = 0;
   seq.subdir_not_notify = 0;
   seq.message[0] = 0;
   seq.message_wait = FALSE;
   seq.stop_after_run = FALSE;
}

/*------------------------------------------------------------------*/

static void seq_start(SEQUENCER& seq, SeqCon* c, bool debug) {
   seq_read(&seq, c);
   seq_clear(seq);

   // manage sequencer parameters and variables

   midas::odb defaults(std::string("/") + c->odb_path + "/Param/Defaults");
   midas::odb param(std::string("/") + c->odb_path + "/Param/Value");
   midas::odb vars(std::string("/") + c->odb_path + "/Variables");

   // check if /Param/Value is there
   for (midas::odb& d: defaults)
      if (!param.is_subkey(d.get_name())) {
         strlcpy(seq.error, "Cannot start script because /Sequencer/Param/Value is incomplete", sizeof(seq.error));
         seq_write(seq, c);
         cm_msg(MERROR, "sequencer", "Cannot start script because /Sequencer/Param/Value is incomplete");
         return;
      }

   // copy /Sequencer/Param/Value to /Sequencer/Variables
   for (midas::odb& p: param)
      vars[p.get_name()] = p.s();

   if (!c->pnseq) {
      strlcpy(seq.error, "Cannot start script, no script loaded", sizeof(seq.error));
      seq_write(seq, c);
      return;
   }

   // start sequencer
   seq.running = TRUE;
   seq.debug = debug;
   seq.current_line_number = 1;
   seq.scurrent_line_number = 1;
   strlcpy(seq.sfilename, seq.filename, sizeof(seq.sfilename));
   seq_write(seq, c);

   if (debug)
      cm_msg(MTALK, "sequencer", "Sequencer started with script \"%s\" in debugging mode.", seq.filename);
   else
      cm_msg(MTALK, "sequencer", "Sequencer started with script \"%s\".", seq.filename);
}

/*------------------------------------------------------------------*/

static void seq_stop(SEQUENCER& seq, SeqCon* c) {
   seq_read(&seq, c);

   if (seq.follow_libraries) {
      std::string path(cm_get_path());
      path += "userfiles/sequencer/";
      path += seq.path;
      path += seq.filename;

      loadMSL(c->odbs, path);
   }

   seq_clear(seq);
   seq.finished = TRUE;
   seq_write(seq, c);
}

/*------------------------------------------------------------------*/

static void seq_stop_after_run(SEQUENCER& seq, SeqCon* c) {
   seq_read(&seq, c);
   seq.stop_after_run = true;
   seq_write(seq, c);
}

/*------------------------------------------------------------------*/

static void seq_cancel_stop_after_run(SEQUENCER& seq, SeqCon* c) {
   seq_read(&seq, c);
   seq.stop_after_run = false;
   seq_write(seq, c);
}

/*------------------------------------------------------------------*/

static void seq_pause(SEQUENCER& seq, SeqCon* c, bool flag) {
   seq_read(&seq, c);
   seq.paused = flag;
   if (!flag)
      seq.debug = false;
   seq_write(seq, c);
}

static void seq_step_over(SEQUENCER& seq, SeqCon* c) {
   seq_read(&seq, c);
   seq.paused = false;
   seq.debug = true;
   seq_write(seq, c);
}

/*------------------------------------------------------------------*/

static void seq_open_file(const char *str, SEQUENCER &seq, SeqCon* c) {
   seq.new_file = FALSE;
   seq.error[0] = 0;
   seq.error_line = 0;
   seq.serror_line = 0;
   if (c->pnseq) {
      mxml_free_tree(c->pnseq);
      c->pnseq = NULL;
   }
   c->odbs->WS("Script/Lines", "");

   if (stristr(str, ".msl")) {
      int size = strlen(str) + 1;
      char *xml_filename = (char *) malloc(size);
      strlcpy(xml_filename, str, size);
      strsubst(xml_filename, size, ".msl", ".xml");
      //printf("Parsing MSL sequencer file: %s to XML sequencer file %s\n", str, xml_filename);

      std::string path(cm_get_path());
      path += "userfiles/sequencer/";

      if (xml_filename[0] == '/') {
         path += xml_filename;
         path = path.substr(0, path.find_last_of('/') + 1);
         strlcpy(xml_filename, path.substr(path.find_last_of('/') + 1).c_str(), size);
      }  else {
         path += seq.path;
      }

      if (msl_parse(seq, c, 0, path.c_str(), str, xml_filename, seq.error, sizeof(seq.error), &seq.serror_line)) {
         //printf("Loading XML sequencer file: %s\n", xml_filename);
         std::string fn(path);
         if (fn.length() > 0 && fn.back() != '/')
            fn += '/';
         fn += xml_filename;
         c->pnseq = mxml_parse_file(fn.c_str(), seq.error, sizeof(seq.error), &seq.error_line);
      } else {
         //printf("Error in MSL sequencer file \"%s\" line %d, error: %s\n", str, seq.serror_line, seq.error);
      }
      free(xml_filename);
   } else {
      //printf("Loading XML sequencer file: %s\n", str);
      c->pnseq = mxml_parse_file(str, seq.error, sizeof(seq.error), &seq.error_line);
   }
}

/*------------------------------------------------------------------*/

static void seq_start_next(SEQUENCER &seq, SeqCon* c) {
   seq_read(&seq, c);
   if (seq.next_filename[0][0]) {
      strlcpy(seq.filename, seq.next_filename[0], sizeof(seq.filename));
      for (int i=0 ; i<9 ; i++)
         strlcpy(seq.next_filename[i], seq.next_filename[i+1], 256);
      seq.next_filename[9][0] = 0;
      seq_write(seq, c);

      seq_open_file(seq.filename, seq, c);

      seq_start(seq, c, false);
   }
}

/*------------------------------------------------------------------*/

static void seq_watch(HNDLE hDB, HNDLE hKeyChanged, int index, void *info) {
   char str[256];
   SeqCon* c = (SeqCon*)info;

   seq_read(c->seqp, c);

   if (c->seqp->new_file) {
      strlcpy(str, c->seqp->path, sizeof(str));
      if (strlen(str) > 1 && str[strlen(str) - 1] != DIR_SEPARATOR)
         strlcat(str, DIR_SEPARATOR_STR, sizeof(str));
      strlcat(str, c->seqp->filename, sizeof(str));

      //printf("Load file %s\n", str);

      seq_open_file(str, *(c->seqp), c);

      seq_clear(*(c->seqp));

      seq_write(*(c->seqp), c);
   }
}

static void seq_watch_command(HNDLE hDB, HNDLE hKeyChanged, int index, void *info) {
   SeqCon* c = (SeqCon*)info;
   SEQUENCER* seqp = c->seqp;

   bool start_script = false;
   bool stop_immediately = false;
   bool stop_after_run = false;
   bool cancel_stop_after_run = false;
   bool pause_script = false;
   bool resume_script = false;
   bool debug_script = false;
   bool step_over = false;
   bool load_new_file = false;

   c->odbs->RB("Command/Start script", &start_script);
   c->odbs->RB("Command/Stop immediately", &stop_immediately);
   c->odbs->RB("Command/Stop after run", &stop_after_run);
   c->odbs->RB("Command/Cancel stop after run", &cancel_stop_after_run);
   c->odbs->RB("Command/Pause script", &pause_script);
   c->odbs->RB("Command/Resume script", &resume_script);
   c->odbs->RB("Command/Debug script", &debug_script);
   c->odbs->RB("Command/Step over", &step_over);
   c->odbs->RB("Command/Load new file", &load_new_file);

   if (load_new_file) {
      std::string filename;
      c->odbs->RS("Command/Load filename", &filename);

      if (filename.find("..") != std::string::npos) {
         strlcpy(seqp->error, "Cannot load \"", sizeof(seqp->error));
         strlcat(seqp->error, filename.c_str(), sizeof(seqp->error));
         strlcat(seqp->error, "\": file names with \"..\" is not permitted", sizeof(seqp->error));
         seq_write(*seqp, c);
      } else if (filename.find(".msl") == std::string::npos) {
         strlcpy(seqp->error, "Cannot load \"", sizeof(seqp->error));
         strlcat(seqp->error, filename.c_str(), sizeof(seqp->error));
         strlcat(seqp->error, "\": file name should end with \".msl\"", sizeof(seqp->error));
         seq_write(*seqp, c);
      } else {
         strlcpy(seqp->filename, filename.c_str(), sizeof(seqp->filename));
         std::string path = cm_expand_env(seqp->path);
         if (path.length() > 0 && path.back() != '/') {
            path += "/";
         }
         HNDLE hDB;
         cm_get_experiment_database(&hDB, NULL);
         seq_clear(*seqp);
         seq_open_file(filename.c_str(), *seqp, c);
         seq_write(*seqp, c);
      }

      // indicate that we successfully load the new file
      c->odbs->WB("Command/Load new file", false);
   }

   if (start_script) {
      c->odbs->WB("Command/Start script", false);

      bool seq_running = false;
      c->odbs->RB("State/running", &seq_running);

      if (!seq_running) {
         seq_start(*seqp, c, false);
      } else {
         cm_msg(MTALK, "sequencer", "Sequencer is already running");
      }
   }

   if (stop_immediately) {
      c->odbs->WB("Command/Stop immediately", false);
      seq_stop(*seqp, c);
      cm_msg(MTALK, "sequencer", "Sequencer is finished by \"stop immediately\".");
   }

   if (stop_after_run) {
      c->odbs->WB("Command/Stop after run", false);
      seq_stop_after_run(*seqp, c);
   }

   if (cancel_stop_after_run) {
      c->odbs->WB("Command/Cancel stop after run", false);
      seq_cancel_stop_after_run(*seqp, c);
   }

   if (pause_script) {
      seq_pause(*seqp, c, true);
      c->odbs->WB("Command/Pause script", false);
      cm_msg(MTALK, "sequencer", "Sequencer is paused.");
   }

   if (resume_script) {
      seq_pause(*seqp, c, false);
      c->odbs->WB("Command/Resume script", false);
      cm_msg(MTALK, "sequencer", "Sequencer is resumed.");
   }

   if (debug_script) {
      c->odbs->WB("Command/Debug script", false);

      bool seq_running = false;
      c->odbs->RB("State/running", &seq_running);

      if (!seq_running) {
         seq_start(*seqp, c, true);
      } else {
         cm_msg(MTALK, "sequencer", "Sequencer is already running");
      }
   }

   if (step_over) {
      seq_step_over(*seqp, c);
      c->odbs->WB("Command/Step over", false);
   }
}

/*------------------------------------------------------------------*/

//performs array index extraction including sequencer variables
void seq_array_index(SEQUENCER& seq, SeqCon* c, char *odbpath, int *index1, int *index2) {
   char str[256];
   *index1 = *index2 = 0;
   if (odbpath[strlen(odbpath) - 1] == ']') {
      if (strchr(odbpath, '[')) {
         //check for sequencer variables
         if (strchr((strchr(odbpath, '[') + 1), '$')) {
            strlcpy(str, strchr(odbpath, '[') + 1, sizeof(str));
            if (strchr(str, ']'))
               *strchr(str, ']') = 0;
            *index1 = atoi(eval_var(seq, c, str).c_str());

            *strchr(odbpath, '[') = 0;
         } else {
            //standard expansion
            strarrayindex(odbpath, index1, index2);
         }
      }
   }
}

/*------------------------------------------------------------------*/

//set all matching keys to a value
int set_all_matching(HNDLE hDB, HNDLE hBaseKey, char *odbpath, char *value, int index1, int index2, int notify) {
   int status, size;
   char data[256];
   KEY key;

   std::vector<HNDLE> keys;
   status = db_find_keys(hDB, hBaseKey, odbpath, keys);

   if (status != DB_SUCCESS)
      return status;

   for (HNDLE hKey: keys) {
      db_get_key(hDB, hKey, &key);
      size = sizeof(data);
      db_sscanf(value, data, &size, 0, key.type);

      /* extend data size for single string if necessary */
      if ((key.type == TID_STRING || key.type == TID_LINK)
          && (int) strlen(data) + 1 > key.item_size && key.num_values == 1)
         key.item_size = strlen(data) + 1;

      if (key.num_values > 1 && index1 == -1) {
         for (int i = 0; i < key.num_values; i++)
            status = db_set_data_index1(hDB, hKey, data, key.item_size, i, key.type, notify);
      } else if (key.num_values > 1 && index2 > index1) {
         for (int i = index1; i < key.num_values && i <= index2; i++)
            status = db_set_data_index1(hDB, hKey, data, key.item_size, i, key.type, notify);
      } else {
         if (key.num_values > 1) {
            status = db_set_data_index1(hDB, hKey, data, key.item_size, index1, key.type, notify);
         } else {
            if (notify)
               status = db_set_data(hDB, hKey, data, key.item_size, 1, key.type);
            else
               status = db_set_data1(hDB, hKey, data, key.item_size, 1, key.type);
         }
      }

      if (status != DB_SUCCESS) {
         return status;
      }
   }

   return DB_SUCCESS;
}

/*------------------------------------------------------------------*/

void sequencer(SEQUENCER& seq, SeqCon* c) {
   PMXML_NODE pn, pr, pt, pe;
   char odbpath[256], value[256], data[256], str[1024], name[32], op[32];
   char list[100][XNAME_LENGTH];
   int i, j, l, n, status, size, index1, index2, state, run_number, cont;
   HNDLE hKey, hKeySeq;
   KEY key;
   double d;
   BOOL skip_step = FALSE;

   if (!seq.running || seq.paused) {
      ss_sleep(10);
      return;
   }

   if (c->pnseq == NULL) {
      seq_stop(seq, c);
      strlcpy(seq.error, "No script loaded", sizeof(seq.error));
      seq_write(seq, c);
      ss_sleep(10);
      return;
   }

   db_find_key(c->hDB, c->hSeq, "State", &hKeySeq);
   if (!hKeySeq)
      return;

   // Retrieve last midas message and put it into seq structure (later sent to ODB via db_set_record()
   cm_msg_retrieve(1, str, sizeof(str));
   str[19] = 0;
   strcpy(seq.last_msg, str+11);

   pr = mxml_find_node(c->pnseq, "RunSequence");
   if (!pr) {
      seq_error(seq, c, "Cannot find &lt;RunSequence&gt; tag in XML file");
      return;
   }

   int last_line = mxml_get_line_number_end(pr);

   /* check for Subroutine end */
   if (seq.stack_index > 0 && seq.current_line_number == seq.subroutine_end_line[seq.stack_index - 1]) {
      size = sizeof(seq);
      db_get_record(c->hDB, hKeySeq, &seq, &size, 0);
      seq.subroutine_end_line[seq.stack_index - 1] = 0;
      seq.current_line_number = seq.subroutine_return_line[seq.stack_index - 1];
      seq.subroutine_return_line[seq.stack_index - 1] = 0;
      seq.subroutine_call_line[seq.stack_index - 1] = 0;
      seq.ssubroutine_call_line[seq.stack_index - 1] = 0;
      seq.stack_index--;
      db_set_record(c->hDB, hKeySeq, &seq, sizeof(seq), 0);
      return;
   }

   /* check for last line of script */
   if (seq.current_line_number > last_line) {
      seq_stop(seq, c);
      cm_msg(MTALK, "sequencer", "Sequencer is finished.");

      seq_start_next(seq, c);
      return;
   }

   /* check for loop end */
   for (i = SEQ_NEST_LEVEL_LOOP-1; i >= 0; i--)
      if (seq.loop_start_line[i] > 0)
         break;
   if (i >= 0) {
      if (seq.current_line_number == seq.loop_end_line[i]) {
         size = sizeof(seq);
         db_get_record(c->hDB, hKeySeq, &seq, &size, 0);

         if (seq.loop_counter[i] == seq.loop_n[i]) {
            seq.loop_counter[i] = 0;
            seq.loop_start_line[i] = 0;
            seq.sloop_start_line[i] = 0;
            seq.loop_end_line[i] = 0;
            seq.sloop_end_line[i] = 0;
            seq.loop_n[i] = 0;
            seq.current_line_number++;
         } else {
            pn = mxml_get_node_at_line(c->pnseq, seq.loop_start_line[i]);
            if (mxml_get_attribute(pn, "var")) {
               strlcpy(name, mxml_get_attribute(pn, "var"), sizeof(name));
               if (mxml_get_attribute(pn, "values")) {
                  strlcpy(data, mxml_get_attribute(pn, "values"), sizeof(data));
                  strbreak(data, list, 100, ",", FALSE);
                  strlcpy(value, eval_var(seq, c, list[seq.loop_counter[i]]).c_str(), sizeof(value));
               } else if (mxml_get_attribute(pn, "n")) {
                  sprintf(value, "%d", seq.loop_counter[i] + 1);
               }
               sprintf(str, "Variables/%s", name);
               size = strlen(value) + 1;
               if (size < 32)
                  size = 32;
               db_set_value(c->hDB, c->hSeq, str, value, size, 1, TID_STRING);
            }
            seq.loop_counter[i]++;
            seq.current_line_number = seq.loop_start_line[i] + 1;
         }
         db_set_record(c->hDB, hKeySeq, &seq, sizeof(seq), 0);
         return;
      }
   }

   /* check for end of "if" statement */
   if (seq.if_index > 0 && seq.current_line_number == seq.if_endif_line[seq.if_index - 1]) {
      size = sizeof(seq);
      db_get_record(c->hDB, hKeySeq, &seq, &size, 0);
      seq.if_index--;
      seq.if_line[seq.if_index] = 0;
      seq.if_else_line[seq.if_index] = 0;
      seq.if_endif_line[seq.if_index] = 0;
      seq.current_line_number++;
      db_set_record(c->hDB, hKeySeq, &seq, sizeof(seq), 0);
      return;
   }

   /* check for ODBSubdir end */
   if (seq.current_line_number == seq.subdir_end_line) {
      size = sizeof(seq);
      db_get_record(c->hDB, hKeySeq, &seq, &size, 0);
      seq.subdir_end_line = 0;
      seq.subdir[0] = 0;
      seq.subdir_not_notify = FALSE;
      db_set_record(c->hDB, hKeySeq, &seq, sizeof(seq), 0);
      return;
   }

   /* find node belonging to current line */
   pn = mxml_get_node_at_line(c->pnseq, seq.current_line_number);
   if (!pn) {
      size = sizeof(seq);
      db_get_record(c->hDB, hKeySeq, &seq, &size, 0);
      seq.current_line_number++;
      db_set_record(c->hDB, hKeySeq, &seq, sizeof(seq), 0);
      return;
   }

   /* set MSL line from current element and put script into ODB if changed (library call) */
   pn = mxml_get_node_at_line(c->pnseq, seq.current_line_number);
   if (pn) {
      if (seq.follow_libraries) {
         if (mxml_get_attribute(pn, "l"))
            seq.scurrent_line_number = atoi(mxml_get_attribute(pn, "l"));
         if (mxml_get_attribute(pn, "fn")) {
            std::string filename = mxml_get_attribute(pn, "fn");

            // load file into ODB if changed
            if (filename != std::string(seq.sfilename)) {
               if (loadMSL(c->odbs, filename))
                  strlcpy(seq.sfilename, filename.c_str(), sizeof(seq.sfilename));
            }
         }
      } else {
         if (mxml_get_attribute(pn, "l") && (!mxml_get_attribute(pn, "lvl") || atoi(mxml_get_attribute(pn, "lvl")) == 0))
            seq.scurrent_line_number = atoi(mxml_get_attribute(pn, "l"));
      }
   }


   // out-comment following lines for debug output
#if 0
   if (seq.scurrent_line_number >= 0) {
      midas::odb o("/" + c->odb_path + "/Script/Lines");
      std::string s = o[seq.scurrent_line_number - 1];
      printf("%3d: %s\n", seq.scurrent_line_number, s.c_str());
   }
#endif

   if (equal_ustring(mxml_get_name(pn), "PI") || equal_ustring(mxml_get_name(pn), "RunSequence") ||
       equal_ustring(mxml_get_name(pn), "Comment")) {
      // just skip
      seq.current_line_number++;
      skip_step = TRUE;
   }

   /*---- ODBSubdir ----*/
   else if (equal_ustring(mxml_get_name(pn), "ODBSubdir")) {
      if (!mxml_get_attribute(pn, "path")) {
         seq_error(seq, c, "Missing attribute \"path\"");
      } else {
         strlcpy(seq.subdir, mxml_get_attribute(pn, "path"), sizeof(seq.subdir));
         if (mxml_get_attribute(pn, "notify"))
            seq.subdir_not_notify = !atoi(mxml_get_attribute(pn, "notify"));
         seq.subdir_end_line = mxml_get_line_number_end(pn);
         seq.current_line_number++;
      }
   }

   /*---- ODBSet ----*/
   else if (equal_ustring(mxml_get_name(pn), "ODBSet")) {
      if (!mxml_get_attribute(pn, "path")) {
         seq_error(seq, c, "Missing attribute \"path\"");
      } else {
         strlcpy(odbpath, seq.subdir, sizeof(odbpath));
         if (strlen(odbpath) > 0 && odbpath[strlen(odbpath) - 1] != '/')
            strlcat(odbpath, "/", sizeof(odbpath));
         strlcat(odbpath, mxml_get_attribute(pn, "path"), sizeof(odbpath));

         if (strchr(odbpath, '$')) {
            if (strchr(odbpath, '[')) {
               // keep $ in index for later evaluation
               std::string s(odbpath);
               std::string s1 = s.substr(0, s.find('['));
               std::string s2 = s.substr(s.find('['));
               s1 = eval_var(seq, c, s1);
               s1 += s2;
               strlcpy(odbpath, s1.c_str(), sizeof(odbpath));
            } else {
               // evaluate variable in path
               std::string s(odbpath);
               s = eval_var(seq, c, s);
               strlcpy(odbpath, s.c_str(), sizeof(odbpath));
            }
         }

         int notify = TRUE;
         if (seq.subdir_not_notify)
            notify = FALSE;
         if (mxml_get_attribute(pn, "notify"))
            notify = atoi(mxml_get_attribute(pn, "notify"));

         index1 = index2 = 0;
         seq_array_index(seq, c, odbpath, &index1, &index2);

         strlcpy(value, eval_var(seq, c, mxml_get_value(pn)).c_str(), sizeof(value));

         status = set_all_matching(c->hDB, 0, odbpath, value, index1, index2, notify);

         if (status == DB_SUCCESS) {
            size = sizeof(seq);
            db_get_record1(c->hDB, hKeySeq, &seq, &size, 0, strcomb1(sequencer_str).c_str());// could have changed seq tree
            seq.current_line_number++;
         } else if (status == DB_NO_KEY) {
            sprintf(str, "ODB key \"%s\" not found", odbpath);
            seq_error(seq, c, str);
         } else {
            //something went really wrong
            sprintf(str, "Internal error %d", status);
            seq_error(seq, c, str);
            return;
         }
      }
   }

   /*---- ODBLoad ----*/
   else if (equal_ustring(mxml_get_name(pn), "ODBLoad")) {
      if (mxml_get_value(pn)[0] == '/') {

         // path relative to the one set in <exp>/userfiles/sequencer
         std::string path(cm_get_path());
         path += "userfiles/sequencer/";
         strlcpy(value, path.c_str(), sizeof(value));
         strlcat(value, mxml_get_value(pn)+1, sizeof(value));

      } else {
         // relative path to msl file
         std::string path(cm_get_path());
         path += "userfiles/sequencer/";
         path += seq.path;
         strlcpy(value, path.c_str(), sizeof(value));
         strlcat(value, seq.filename, sizeof(value));
         char *fullpath = strrchr(value, '/');
         if (fullpath)
            *(++fullpath) = '\0';
         strlcat(value, mxml_get_value(pn), sizeof(value));
      }

      // if path attribute is given
      if (mxml_get_attribute(pn, "path")) {
         strlcpy(odbpath, seq.subdir, sizeof(odbpath));
         if (strlen(odbpath) > 0 && odbpath[strlen(odbpath) - 1] != '/')
            strlcat(odbpath, "/", sizeof(odbpath));
         strlcat(odbpath, mxml_get_attribute(pn, "path"), sizeof(odbpath));

         if (strchr(odbpath, '$')) {
            std::string s(odbpath);
            s = eval_var(seq, c, s);
            strlcpy(odbpath, s.c_str(), sizeof(odbpath));
         }

         //load at that key, if exists
         status = db_find_key(c->hDB, 0, odbpath, &hKey);
         if (status != DB_SUCCESS) {
            char errorstr[512];
            sprintf(errorstr, "Cannot find ODB key \"%s\"", odbpath);
            seq_error(seq, c, errorstr);
            return;
         } else {
            status = db_load(c->hDB, hKey, value, FALSE);
         }
      } else {
         //otherwise load at root
         status = db_load(c->hDB, 0, value, FALSE);
      }

      if (status == DB_SUCCESS) {
         size = sizeof(seq);
         db_get_record1(c->hDB, hKeySeq, &seq, &size, 0, strcomb1(sequencer_str).c_str());// could have changed seq tree
         seq.current_line_number++;
      } else if (status == DB_FILE_ERROR) {
         sprintf(str, "Error reading file \"%s\"", value);
         seq_error(seq, c, str);
      } else {
         //something went really wrong
         seq_error(seq, c, "Internal error loading ODB file!");
         return;
      }
   }

   /*---- ODBSave ----*/
   else if (equal_ustring(mxml_get_name(pn), "ODBSave")) {
      if (mxml_get_value(pn)[0] == '/') {

         // path relative to the one set in <exp>/userfiles/sequencer
         std::string path(cm_get_path());
         path += "userfiles/sequencer/";
         strlcpy(value, path.c_str(), sizeof(value));
         strlcat(value, mxml_get_value(pn)+1, sizeof(value));

      } else {
         // relative path to msl file
         std::string path(cm_get_path());
         path += "userfiles/sequencer/";
         path += seq.path;
         strlcpy(value, path.c_str(), sizeof(value));
         strlcat(value, seq.filename, sizeof(value));
         char *fullpath = strrchr(value, '/');
         if (fullpath)
            *(++fullpath) = '\0';
         strlcat(value, mxml_get_value(pn), sizeof(value));
      }

      if (strchr(value, '$')) {
         std::string s(value);
         s = eval_var(seq, c, s);
         strlcpy(value, s.c_str(), sizeof(value));
      }

      // if path attribute is given
      if (mxml_get_attribute(pn, "path") && *mxml_get_attribute(pn, "path")) {
         strlcpy(odbpath, seq.subdir, sizeof(odbpath));
         if (strlen(odbpath) > 0 && odbpath[strlen(odbpath) - 1] != '/')
            strlcat(odbpath, "/", sizeof(odbpath));
         strlcat(odbpath, mxml_get_attribute(pn, "path"), sizeof(odbpath));

         if (strchr(odbpath, '$')) {
            std::string s(odbpath);
            s = eval_var(seq, c, s);
            strlcpy(odbpath, s.c_str(), sizeof(odbpath));
         }

         // find key or subdirectory to save
         status = db_find_key(c->hDB, 0, odbpath, &hKey);
         if (status != DB_SUCCESS) {
            char errorstr[512];
            sprintf(errorstr, "Cannot find ODB key \"%s\"", odbpath);
            seq_error(seq, c, errorstr);
            return;
         } else {
            if (strstr(value, ".json") || strstr(value, ".JSON") || strstr(value, ".js") || strstr(value, ".JS"))
               status = db_save_json(c->hDB, hKey, value, JSFLAG_RECURSE | JSFLAG_OMIT_LAST_WRITTEN | JSFLAG_FOLLOW_LINKS);
            else if  (strstr(value, ".xml") || strstr(value, ".XML"))
               status = db_save_xml(c->hDB, hKey, value);
            else
               status = db_save(c->hDB, hKey, value, FALSE);
            if (status != DB_SUCCESS) {
               char err[1024];
               sprintf(err, "Cannot save file \"%s\", error %d", value, status);
               seq_error(seq, c, err);
               return;
            }
         }
      } else {
         seq_error(seq, c, "No ODB path specified in ODBSAVE command");
         return;
      }

      if (status == DB_SUCCESS) {
         size = sizeof(seq);
         db_get_record1(c->hDB, hKeySeq, &seq, &size, 0, strcomb1(sequencer_str).c_str());// could have changed seq tree
         seq.current_line_number++;
      } else if (status == DB_FILE_ERROR) {
         sprintf(str, "Error reading file \"%s\"", value);
         seq_error(seq, c, str);
      } else {
         //something went really wrong
         seq_error(seq, c, "Internal error loading ODB file!");
         return;
      }
   }

   /*---- ODBGet ----*/
   else if (equal_ustring(mxml_get_name(pn), "ODBGet")) {
      if (!mxml_get_attribute(pn, "path")) {
         seq_error(seq, c, "Missing attribute \"path\"");
      } else {
         strlcpy(odbpath, seq.subdir, sizeof(odbpath));
         if (strlen(odbpath) > 0 && odbpath[strlen(odbpath) - 1] != '/')
            strlcat(odbpath, "/", sizeof(odbpath));
         strlcat(odbpath, mxml_get_attribute(pn, "path"), sizeof(odbpath));

         if (strchr(odbpath, '$')) {
            if (strchr(odbpath, '[')) {
               // keep $ in index for later evaluation
               std::string s(odbpath);
               std::string s1 = s.substr(0, s.find('['));
               std::string s2 = s.substr(s.find('['));
               s1 = eval_var(seq, c, s1);
               s1 += s2;
               strlcpy(odbpath, s1.c_str(), sizeof(odbpath));
            } else {
               // evaluate variable in path
               std::string s(odbpath);
               s = eval_var(seq, c, s);
               strlcpy(odbpath, s.c_str(), sizeof(odbpath));
            }
         }

         /* check if index is supplied */
         index1 = index2 = 0;
         seq_array_index(seq, c, odbpath, &index1, &index2);

         strlcpy(name, mxml_get_value(pn), sizeof(name));
         status = db_find_key(c->hDB, 0, odbpath, &hKey);
         if (status != DB_SUCCESS) {
            char errorstr[512];
            sprintf(errorstr, "Cannot find ODB key \"%s\"", odbpath);
            seq_error(seq, c, errorstr);
            return;
         } else {
            db_get_key(c->hDB, hKey, &key);
            size = sizeof(data);

            status = db_get_data_index(c->hDB, hKey, data, &size, index1, key.type);
            if (key.type == TID_BOOL)
               strlcpy(value, *((int *) data) > 0 ? "1" : "0", sizeof(value));
            else
               db_sprintf(value, data, size, 0, key.type);

            sprintf(str, "Variables/%s", name);
            size = strlen(value) + 1;
            if (size < 32)
               size = 32;
            db_set_value(c->hDB, c->hSeq, str, value, size, 1, TID_STRING);

            size = sizeof(seq);
            db_get_record1(c->hDB, hKeySeq, &seq, &size, 0, strcomb1(sequencer_str).c_str());// could have changed seq tree
            seq.current_line_number = mxml_get_line_number_end(pn) + 1;
         }
      }
   }

   /*---- ODBInc ----*/
   else if (equal_ustring(mxml_get_name(pn), "ODBInc")) {
      if (!mxml_get_attribute(pn, "path")) {
         seq_error(seq, c, "Missing attribute \"path\"");
      } else {
         strlcpy(odbpath, seq.subdir, sizeof(odbpath));
         if (strlen(odbpath) > 0 && odbpath[strlen(odbpath) - 1] != '/')
            strlcat(odbpath, "/", sizeof(odbpath));
         strlcat(odbpath, mxml_get_attribute(pn, "path"), sizeof(odbpath));

         if (strchr(odbpath, '$')) {
            if (strchr(odbpath, '[')) {
               // keep $ in index for later evaluation
               std::string s(odbpath);
               std::string s1 = s.substr(0, s.find('['));
               std::string s2 = s.substr(s.find('['));
               s1 = eval_var(seq, c, s1);
               s1 += s2;
               strlcpy(odbpath, s1.c_str(), sizeof(odbpath));
            } else {
               // evaluate variable in path
               std::string s(odbpath);
               s = eval_var(seq, c, s);
               strlcpy(odbpath, s.c_str(), sizeof(odbpath));
            }
         }

         index1 = index2 = 0;
         seq_array_index(seq, c, odbpath, &index1, &index2);

         strlcpy(value, eval_var(seq, c, mxml_get_value(pn)).c_str(), sizeof(value));

         status = db_find_key(c->hDB, 0, odbpath, &hKey);
         if (status != DB_SUCCESS) {
            char errorstr[512];
            sprintf(errorstr, "Cannot find ODB key \"%s\"", odbpath);
            seq_error(seq, c, errorstr);
         } else {
            db_get_key(c->hDB, hKey, &key);
            size = sizeof(data);
            db_get_data_index(c->hDB, hKey, data, &size, index1, key.type);
            db_sprintf(str, data, size, 0, key.type);
            d = atof(str);
            d += atof(value);
            sprintf(str, "%lg", d);
            size = sizeof(data);
            db_sscanf(str, data, &size, 0, key.type);

            int notify = TRUE;
            if (seq.subdir_not_notify)
               notify = FALSE;
            if (mxml_get_attribute(pn, "notify"))
               notify = atoi(mxml_get_attribute(pn, "notify"));

            db_set_data_index1(c->hDB, hKey, data, key.item_size, index1, key.type, notify);
            seq.current_line_number++;
         }
      }
   }

   /*---- ODBDelete ----*/
   else if (equal_ustring(mxml_get_name(pn), "ODBDelete")) {
      strlcpy(odbpath, seq.subdir, sizeof(odbpath));
      if (strlen(odbpath) > 0 && odbpath[strlen(odbpath) - 1] != '/')
         strlcat(odbpath, "/", sizeof(odbpath));
      strlcat(odbpath, mxml_get_value(pn), sizeof(odbpath));

      if (strchr(odbpath, '$')) {
         std::string s(odbpath);
         s = eval_var(seq, c, s);
         strlcpy(odbpath, s.c_str(), sizeof(odbpath));
      }

      status = db_find_key(c->hDB, 0, odbpath, &hKey);
      if (status != DB_SUCCESS) {
         char errorstr[512];
         sprintf(errorstr, "Cannot find ODB key \"%s\"", odbpath);
         seq_error(seq, c, errorstr);
      } else {
         status = db_delete_key(c->hDB, hKey, FALSE);
         if (status != DB_SUCCESS) {
            char errorstr[512];
            sprintf(errorstr, "Cannot delete ODB key \"%s\"", odbpath);
            seq_error(seq, c, errorstr);
         } else
            seq.current_line_number++;
      }
   }

   /*---- ODBCreate ----*/
   else if (equal_ustring(mxml_get_name(pn), "ODBCreate")) {
      if (!mxml_get_attribute(pn, "path")) {
         seq_error(seq, c, "Missing attribute \"path\"");
      } else if (!mxml_get_attribute(pn, "type")) {
         seq_error(seq, c, "Missing attribute \"type\"");
      } else {
         strlcpy(odbpath, seq.subdir, sizeof(odbpath));
         if (strlen(odbpath) > 0 && odbpath[strlen(odbpath) - 1] != '/')
            strlcat(odbpath, "/", sizeof(odbpath));
         strlcat(odbpath, mxml_get_attribute(pn, "path"), sizeof(odbpath));

         if (strchr(odbpath, '$')) {
            std::string s(odbpath);
            s = eval_var(seq, c, s);
            strlcpy(odbpath, s.c_str(), sizeof(odbpath));
         }

         /* get TID */
         unsigned int tid;
         for (tid = 0; tid < TID_LAST; tid++) {
            if (equal_ustring(rpc_tid_name(tid), mxml_get_attribute(pn, "type")))
               break;
         }

         if (tid == TID_LAST)
            seq_error(seq, c, "Type must be one of UINT8,INT8,UINT16,INT16,UINT32,INT32,BOOL,FLOAT,DOUBLE,STRING");
         else {

            status = db_find_key(c->hDB, 0, odbpath, &hKey);
            if (status == DB_SUCCESS) {
               db_get_key(c->hDB, hKey, &key);
               if (key.type != tid) {
                  db_delete_key(c->hDB, hKey, FALSE);
                  status = db_create_key(c->hDB, 0, odbpath, tid);
               }
            } else {
               status = db_create_key(c->hDB, 0, odbpath, tid);
               char dummy[32];
               memset(dummy, 0, sizeof(dummy));
               if (tid == TID_STRING || tid == TID_LINK)
                  db_set_value(c->hDB, 0, odbpath, dummy, 32, 1, tid);
            }

            if (status != DB_SUCCESS && status != DB_CREATED) {
               char errorstr[512];
               sprintf(errorstr, "Cannot createODB key \"%s\", error code %d", odbpath, status);
               seq_error(seq, c, errorstr);
            } else {
               status = db_find_key(c->hDB, 0, odbpath, &hKey);
               if (mxml_get_attribute(pn, "size")) {
                  i = atoi(eval_var(seq, c, mxml_get_attribute(pn, "size")).c_str());
                  if (i > 1)
                     db_set_num_values(c->hDB, hKey, i);
               }
               seq.current_line_number++;
            }
         }
      }
   }

   /*---- RunDescription ----*/
   else if (equal_ustring(mxml_get_name(pn), "RunDescription")) {
      db_set_value(c->hDB, 0, "/Experiment/Run Parameters/Run Description", mxml_get_value(pn), 256, 1, TID_STRING);
      seq.current_line_number++;
   }

   /*---- Script ----*/
   else if (equal_ustring(mxml_get_name(pn), "Script")) {
      sprintf(str, "%s", mxml_get_value(pn));

      if (mxml_get_attribute(pn, "params")) {
         strlcpy(data, mxml_get_attribute(pn, "params"), sizeof(data));
         n = strbreak(data, list, 100, ",", FALSE);
         for (i = 0; i < n; i++) {

            strlcpy(value, eval_var(seq, c, list[i]).c_str(), sizeof(value));

            strlcat(str, " ", sizeof(str));
            strlcat(str, value, sizeof(str));
         }
      }

      std::string s(str);
      s = ss_replace_env_variables(s);
      std::string r = ss_execs(s.c_str());

      // put result into SCRIPT_RESULT variable
      db_set_value(c->hDB, c->hSeq, "Variables/SCRIPT_RESULT", r.c_str(), r.length(), 1, TID_STRING);

      seq.current_line_number++;
   }

   /*---- Transition ----*/
   else if (equal_ustring(mxml_get_name(pn), "Transition")) {
      if (equal_ustring(mxml_get_value(pn), "Start")) {
         if (!seq.transition_request) {
            seq.transition_request = TRUE;
            size = sizeof(state);
            db_get_value(c->hDB, 0, "/Runinfo/State", &state, &size, TID_INT32, FALSE);
            if (state != STATE_RUNNING) {
               size = sizeof(run_number);
               db_get_value(c->hDB, 0, "/Runinfo/Run number", &run_number, &size, TID_INT32, FALSE);
               status = cm_transition(TR_START, run_number + 1, str, sizeof(str), TR_MTHREAD | TR_SYNC, FALSE);
               if (status != CM_SUCCESS) {
                  char errorstr[1500];
                  sprintf(errorstr, "Cannot start run: %s", str);
                  seq_error(seq, c, errorstr);
               }
            }
         } else {
            // Wait until transition has finished
            size = sizeof(state);
            db_get_value(c->hDB, 0, "/Runinfo/State", &state, &size, TID_INT32, FALSE);
            if (state == STATE_RUNNING) {
               seq.transition_request = FALSE;
               seq.current_line_number++;
            }
         }
      } else if (equal_ustring(mxml_get_value(pn), "Stop")) {
         if (!seq.transition_request) {
            seq.transition_request = TRUE;
            size = sizeof(state);
            db_get_value(c->hDB, 0, "/Runinfo/State", &state, &size, TID_INT32, FALSE);
            if (state != STATE_STOPPED) {
               status = cm_transition(TR_STOP, 0, str, sizeof(str), TR_MTHREAD | TR_SYNC, FALSE);
               if (status == CM_DEFERRED_TRANSITION) {
                  // do nothing
               } else if (status != CM_SUCCESS) {
                  char errorstr[1500];
                  sprintf(errorstr, "Cannot stop run: %s", str);
                  seq_error(seq, c, errorstr);
               }
            }
         } else {
            // Wait until transition has finished
            size = sizeof(state);
            db_get_value(c->hDB, 0, "/Runinfo/State", &state, &size, TID_INT32, FALSE);
            if (state == STATE_STOPPED) {
               size = sizeof(seq);
               db_get_record(c->hDB, hKeySeq, &seq, &size, 0);

               seq.transition_request = FALSE;

               if (seq.stop_after_run) {
                  seq.stop_after_run = FALSE;
                  seq.running = FALSE;
                  seq.finished = TRUE;
                  seq_stop(seq, c);
                  cm_msg(MTALK, "sequencer", "Sequencer is finished by \"stop after current run\".");
               } else {
                  seq.current_line_number++;
               }

               db_set_record(c->hDB, hKeySeq, &seq, sizeof(seq), 0);
            }
         }
      } else {
         sprintf(str, "Invalid transition \"%s\"", mxml_get_value(pn));
         seq_error(seq, c, str);
         return;
      }
   }

   /*---- Wait ----*/
   else if (equal_ustring(mxml_get_name(pn), "Wait")) {
      if (equal_ustring(mxml_get_attribute(pn, "for"), "Events")) {
         n = atoi(eval_var(seq, c, mxml_get_value(pn)).c_str());
         seq.wait_limit = (float) n;
         strcpy(seq.wait_type, "Events");
         size = sizeof(d);
         db_get_value(c->hDB, 0, "/Equipment/Trigger/Statistics/Events sent", &d, &size, TID_DOUBLE, FALSE);
         seq.wait_value = (float) d;
         if (d >= n) {
            seq.current_line_number = mxml_get_line_number_end(pn) + 1;
            seq.wait_limit = 0;
            seq.wait_value = 0;
            seq.wait_type[0] = 0;
            seq.wait_odb[0] = 0;
         }
         seq.wait_value = (float) d;
      } else if (equal_ustring(mxml_get_attribute(pn, "for"), "ODBValue")) {
         seq.wait_limit = (float) atof(eval_var(seq, c, mxml_get_value(pn)).c_str());
         strlcpy(seq.wait_type, "ODB", sizeof(seq.wait_type));
         if (!mxml_get_attribute(pn, "path")) {
            seq_error(seq, c, "\"path\" must be given for ODB values");
            return;
         } else {
            strlcpy(odbpath, mxml_get_attribute(pn, "path"), sizeof(odbpath));
            strlcpy(seq.wait_odb, odbpath, sizeof(seq.wait_odb));

            if (strchr(odbpath, '$')) {
               if (strchr(odbpath, '[')) {
                  // keep $ in index for later evaluation
                  std::string s(odbpath);
                  std::string s1 = s.substr(0, s.find('['));
                  std::string s2 = s.substr(s.find('['));
                  s1 = eval_var(seq, c, s1);
                  s1 += s2;
                  strlcpy(odbpath, s1.c_str(), sizeof(odbpath));
               } else {
                  // evaluate variable in path
                  std::string s(odbpath);
                  s = eval_var(seq, c, s);
                  strlcpy(odbpath, s.c_str(), sizeof(odbpath));
               }
            }

            index1 = index2 = 0;
            seq_array_index(seq, c, odbpath, &index1, &index2);
            status = db_find_key(c->hDB, 0, odbpath, &hKey);
            if (status != DB_SUCCESS) {
               char errorstr[512];
               sprintf(errorstr, "Cannot find ODB key \"%s\"", odbpath);
               seq_error(seq, c, errorstr);
               return;
            } else {
               if (mxml_get_attribute(pn, "op"))
                  strlcpy(op, mxml_get_attribute(pn, "op"), sizeof(op));
               else
                  strcpy(op, "!=");
               strlcat(seq.wait_type, op, sizeof(seq.wait_type));

               db_get_key(c->hDB, hKey, &key);
               size = sizeof(data);
               db_get_data_index(c->hDB, hKey, data, &size, index1, key.type);
               if (key.type == TID_BOOL)
                  strlcpy(str, *((int *) data) > 0 ? "1" : "0", sizeof(str));
               else
                  db_sprintf(str, data, size, 0, key.type);
               cont = FALSE;
               seq.wait_value = (float) atof(str);
               if (equal_ustring(op, ">=")) {
                  cont = (seq.wait_value >= seq.wait_limit);
               } else if (equal_ustring(op, ">")) {
                  cont = (seq.wait_value > seq.wait_limit);
               } else if (equal_ustring(op, "<=")) {
                  cont = (seq.wait_value <= seq.wait_limit);
               } else if (equal_ustring(op, "<")) {
                  cont = (seq.wait_value < seq.wait_limit);
               } else if (equal_ustring(op, "==")) {
                  cont = (seq.wait_value == seq.wait_limit);
               } else if (equal_ustring(op, "!=")) {
                  cont = (seq.wait_value != seq.wait_limit);
               } else {
                  sprintf(str, "Invalid comaprison \"%s\"", op);
                  seq_error(seq, c, str);
                  return;
               }

               if (cont) {
                  seq.current_line_number = mxml_get_line_number_end(pn) + 1;
                  seq.wait_limit = 0;
                  seq.wait_value = 0;
                  seq.wait_type[0] = 0;
                  seq.wait_odb[0] = 0;
               }
            }
         }
      } else if (equal_ustring(mxml_get_attribute(pn, "for"), "Seconds")) {
         seq.wait_limit = 1000.0f * (float) atof(eval_var(seq, c, mxml_get_value(pn)).c_str());
         strcpy(seq.wait_type, "Seconds");
         if (seq.start_time == 0) {
            seq.start_time = ss_millitime();
            seq.wait_value = 0;
         } else {
            seq.wait_value = (float) (ss_millitime() - seq.start_time);
            if (seq.wait_value > seq.wait_limit)
               seq.wait_value = seq.wait_limit;
         }
         if (ss_millitime() - seq.start_time > (DWORD) seq.wait_limit) {
            seq.current_line_number++;
            seq.start_time = 0;
            seq.wait_limit = 0;
            seq.wait_value = 0;
            seq.wait_type[0] = 0;
            seq.wait_odb[0] = 0;
         }
      } else {
         sprintf(str, "Invalid wait attribute \"%s\"", mxml_get_attribute(pn, "for"));
         seq_error(seq, c, str);
      }

      // sleep to keep the CPU from consuming 100%
      ss_sleep(1);
   }

   /*---- Loop start ----*/
   else if (equal_ustring(mxml_get_name(pn), "Loop")) {
      for (i = 0; i < SEQ_NEST_LEVEL_LOOP; i++)
         if (seq.loop_start_line[i] == 0)
            break;
      if (i == SEQ_NEST_LEVEL_LOOP) {
         seq_error(seq, c, "Maximum loop nesting exceeded");
         return;
      }
      seq.loop_start_line[i] = seq.current_line_number;
      seq.loop_end_line[i] = mxml_get_line_number_end(pn);
      if (mxml_get_attribute(pn, "l"))
         seq.sloop_start_line[i] = atoi(mxml_get_attribute(pn, "l"));
      if (mxml_get_attribute(pn, "le"))
         seq.sloop_end_line[i] = atoi(mxml_get_attribute(pn, "le"));
      seq.loop_counter[i] = 1;

      if (mxml_get_attribute(pn, "n")) {
         if (equal_ustring(mxml_get_attribute(pn, "n"), "infinite"))
            seq.loop_n[i] = -1;
         else {
            seq.loop_n[i] = atoi(eval_var(seq, c, mxml_get_attribute(pn, "n")).c_str());
         }
         strlcpy(value, "1", sizeof(value));
      } else if (mxml_get_attribute(pn, "values")) {
         strlcpy(data, mxml_get_attribute(pn, "values"), sizeof(data));
         seq.loop_n[i] = strbreak(data, list, 100, ",", FALSE);
         strlcpy(value, eval_var(seq, c, list[0]).c_str(), sizeof(value));
      } else {
         seq_error(seq, c, "Missing \"var\" or \"n\" attribute");
         return;
      }

      if (mxml_get_attribute(pn, "var")) {
         strlcpy(name, mxml_get_attribute(pn, "var"), sizeof(name));
         sprintf(str, "Variables/%s", name);
         size = strlen(value) + 1;
         if (size < 32)
            size = 32;
         db_set_value(c->hDB, c->hSeq, str, value, size, 1, TID_STRING);
      }

      seq.current_line_number++;
   }

   /*---- Break ----*/
   else if (equal_ustring(mxml_get_name(pn), "Break")) {

      // finish current if statement
      while (seq.if_index > 0 &&
             seq.current_line_number > seq.if_line[seq.if_index - 1] &&
             seq.current_line_number < seq.if_endif_line[seq.if_index-1]) {
         size = sizeof(seq);
         db_get_record(c->hDB, hKeySeq, &seq, &size, 0);
         seq.if_index--;
         seq.if_line[seq.if_index] = 0;
         seq.if_else_line[seq.if_index] = 0;
         seq.if_endif_line[seq.if_index] = 0;
         seq.current_line_number++;
         db_set_record(c->hDB, hKeySeq, &seq, sizeof(seq), 0);
      }

      // goto next loop end
      for (i = 0; i < SEQ_NEST_LEVEL_LOOP; i++)
         if (seq.loop_start_line[i] == 0)
            break;
      if (i == 0) {
         seq_error(seq, c, "\"Break\" outside any loop");
         return;
      }

      // force end of loop in next check
      seq.current_line_number = seq.loop_end_line[i-1];
      seq.loop_counter[i-1] = seq.loop_n[i-1];
   }

   /*---- If ----*/
   else if (equal_ustring(mxml_get_name(pn), "If")) {

      if (seq.if_index == SEQ_NEST_LEVEL_IF) {
         seq_error(seq, c, "Maximum number of nested if..endif exceeded");
         return;
      }

      // store "if" and "endif" lines
      seq.if_line[seq.if_index] = seq.current_line_number;
      seq.if_endif_line[seq.if_index] = mxml_get_line_number_end(pn);

      // search for "else" line
      seq.if_else_line[seq.if_index] = 0;
      for (j = seq.current_line_number + 1; j < mxml_get_line_number_end(pn) + 1; j++) {
         pe = mxml_get_node_at_line(c->pnseq, j);

         // skip nested if..endif
         if (pe && equal_ustring(mxml_get_name(pe), "If")) {
            j = mxml_get_line_number_end(pe);
            continue;
         }

         // store "else" if found
         if (pe && equal_ustring(mxml_get_name(pe), "Else")) {
            seq.if_else_line[seq.if_index] = j;
            break;
         }
      }

      strlcpy(str, mxml_get_attribute(pn, "condition"), sizeof(str));
      i = eval_condition(seq, c, str);
      if (i < 0) {
         seq_error(seq, c, "Invalid number in comparison");
         return;
      }

      if (i == 1)
         seq.current_line_number++;
      else if (seq.if_else_line[seq.if_index])
         seq.current_line_number = seq.if_else_line[seq.if_index] + 1;
      else
         seq.current_line_number = seq.if_endif_line[seq.if_index];

      seq.if_index++;
   }

   /*---- Else ----*/
   else if (equal_ustring(mxml_get_name(pn), "Else")) {
      // goto next "Endif"
      if (seq.if_index == 0) {
         seq_error(seq, c, "Unexpected Else");
         return;
      }
      seq.current_line_number = seq.if_endif_line[seq.if_index - 1];
   }

   /*---- Exit ----*/
   else if (equal_ustring(mxml_get_name(pn), "Exit")) {
      seq_stop(seq, c);
      cm_msg(MTALK, "sequencer", "Sequencer is finished.");
      return;
   }

   /*---- Goto ----*/
   else if (equal_ustring(mxml_get_name(pn), "Goto")) {
      if (!mxml_get_attribute(pn, "line") && !mxml_get_attribute(pn, "sline")) {
         seq_error(seq, c, "Missing line number");
         return;
      }
      if (mxml_get_attribute(pn, "line")) {
         seq.current_line_number = atoi(eval_var(seq, c, mxml_get_attribute(pn, "line")).c_str());
      }
      if (mxml_get_attribute(pn, "sline")) {
         strlcpy(str, eval_var(seq, c, mxml_get_attribute(pn, "sline")).c_str(), sizeof(str));
         for (i = 0; i < last_line; i++) {
            pt = mxml_get_node_at_line(c->pnseq, i);
            if (pt && mxml_get_attribute(pt, "l")) {
               l = atoi(mxml_get_attribute(pt, "l"));
               if (atoi(str) == l) {
                  seq.current_line_number = i;
                  break;
               }
            }
         }
      }
   }

   /*---- Library ----*/
   else if (equal_ustring(mxml_get_name(pn), "Library")) {
      // simply skip libraries
      seq.current_line_number = mxml_get_line_number_end(pn) + 1;
   }

   /*---- Subroutine ----*/
   else if (equal_ustring(mxml_get_name(pn), "Subroutine")) {
      // simply skip subroutines
      seq.current_line_number = mxml_get_line_number_end(pn) + 1;
   }

   /*---- Param ----*/
   else if (equal_ustring(mxml_get_name(pn), "Param")) {
      // simply skip parameters
      seq.current_line_number = mxml_get_line_number_end(pn) + 1;
   }

   /*---- Set ----*/
   else if (equal_ustring(mxml_get_name(pn), "Set")) {
      if (!mxml_get_attribute(pn, "name")) {
         seq_error(seq, c, "Missing variable name");
         return;
      }
      strlcpy(name, mxml_get_attribute(pn, "name"), sizeof(name));
      strlcpy(value, eval_var(seq, c, mxml_get_value(pn)).c_str(), sizeof(value));

      if (strchr(name, '[')) {
         // array
         strlcpy(str, strchr(name, '[')+1, sizeof(str));
         if (strchr(str, ']'))
            *strchr(str, ']') = 0;
         int index = atoi(eval_var(seq, c, str).c_str());
         *strchr(name, '[') = 0;
         sprintf(str, "Variables/%s", name);
         status = db_find_key(c->hDB, c->hSeq, str, &hKey);
         if (status != DB_SUCCESS) {
            db_create_key(c->hDB, c->hSeq, str, TID_STRING);
            status = db_find_key(c->hDB, c->hSeq, str, &hKey);
         }
         size = strlen(value) + 1;
         if (size < 32)
            size = 32;
         status = db_set_data_index(c->hDB, hKey, value, size, index, TID_STRING);
      } else {
         sprintf(str, "Variables/%s", name);
         size = strlen(value) + 1;
         if (size < 32)
            size = 32;
         db_set_value(c->hDB, c->hSeq, str, value, size, 1, TID_STRING);
      }

      // check if variable is used in loop
      for (i = SEQ_NEST_LEVEL_LOOP-1; i >= 0; i--)
         if (seq.loop_start_line[i] > 0) {
            pr = mxml_get_node_at_line(c->pnseq, seq.loop_start_line[i]);
            if (mxml_get_attribute(pr, "var")) {
               if (equal_ustring(mxml_get_attribute(pr, "var"), name))
                  seq.loop_counter[i] = atoi(value);
            }
         }

      seq.current_line_number = mxml_get_line_number_end(pn) + 1;
   }

   /*---- Message ----*/
   else if (equal_ustring(mxml_get_name(pn), "Message")) {
      if (strchr(mxml_get_value(pn), '$')) // evaluate message string if $ present
         strlcpy(value, eval_var(seq, c, mxml_get_value(pn)).c_str(), sizeof(value));
      else
         strlcpy(value, mxml_get_value(pn), sizeof(value)); // treat message as string
      const char *wait_attr = mxml_get_attribute(pn, "wait");
      bool wait = false;
      if (wait_attr)
         wait = (atoi(wait_attr) == 1);

      if (!wait) {
         // message with no wait: set seq.message and move on. we do not care if web page clears it
         strlcpy(seq.message, value, sizeof(seq.message));
         seq.message_wait = FALSE;
         db_set_record(c->hDB, hKeySeq, &seq, sizeof(seq), 0);
      } else {
         // message with wait

         // if message_wait not set, we are here for the first time
         if (!seq.message_wait) {
            strlcpy(seq.message, value, sizeof(seq.message));
            seq.message_wait = TRUE;
            db_set_record(c->hDB, hKeySeq, &seq, sizeof(seq), 0);
            // wait
            return;
         } else {
            // message_wait is set, we have been here before

            // if web page did not clear the message, keep waiting
            if (seq.message[0] != 0) {
               // wait
               return;
            }

            // web page cleared the message, we are done with waiting
            seq.message_wait = false;
         }
      }

      seq.current_line_number = mxml_get_line_number_end(pn) + 1;
   }

   /*---- Msg ----*/
   else if (equal_ustring(mxml_get_name(pn), "Msg")) {
      if (strchr(mxml_get_value(pn), '$')) // evaluate message string if $ present
         strlcpy(value, eval_var(seq, c, mxml_get_value(pn)).c_str(), sizeof(value));
      else
         strlcpy(value, mxml_get_value(pn), sizeof(value)); // treat message as string
      std::string type = "INFO";
      if (mxml_get_attribute(pn, "type"))
         type = std::string(mxml_get_attribute(pn, "type"));

      if (type == "ERROR")
         cm_msg(MERROR, "sequencer", "%s", value);
      else if (type == "DEBUG")
         cm_msg(MDEBUG, "sequencer", "%s", value);
      else if (type == "LOG")
         cm_msg(MLOG, "sequencer", "%s", value);
      else if (type == "TALK")
         cm_msg(MTALK, "sequencer", "%s", value);
      else
         cm_msg(MINFO, "sequencer", "%s", value);

      seq.current_line_number = mxml_get_line_number_end(pn) + 1;
   }

   /*---- Cat ----*/
   else if (equal_ustring(mxml_get_name(pn), "Cat")) {
      if (!mxml_get_attribute(pn, "name")) {
         seq_error(seq, c, "Missing variable name");
         return;
      }
      strlcpy(name, mxml_get_attribute(pn, "name"), sizeof(name));
      if (!concatenate(seq, c, value, sizeof(value), mxml_get_value(pn)))
         return;
      sprintf(str, "Variables/%s", name);
      size = strlen(value) + 1;
      if (size < 32)
         size = 32;
      db_set_value(c->hDB, c->hSeq, str, value, size, 1, TID_STRING);

      seq.current_line_number = mxml_get_line_number_end(pn) + 1;
   }

   /*---- Call ----*/
   else if (equal_ustring(mxml_get_name(pn), "Call")) {
      if (seq.stack_index == SEQ_NEST_LEVEL_SUB) {
         seq_error(seq, c, "Maximum subroutine level exceeded");
         return;
      } else {
         // put current line number on stack
         seq.subroutine_call_line[seq.stack_index] = mxml_get_line_number_end(pn);
         seq.ssubroutine_call_line[seq.stack_index] = atoi(mxml_get_attribute(pn, "l"));
         seq.subroutine_return_line[seq.stack_index] = mxml_get_line_number_end(pn) + 1;

         // search subroutine
         for (i = 1; i < mxml_get_line_number_end(mxml_find_node(c->pnseq, "RunSequence")); i++) {
            pt = mxml_get_node_at_line(c->pnseq, i);
            if (pt) {
               if (equal_ustring(mxml_get_name(pt), "Subroutine")) {
                  if (equal_ustring(mxml_get_attribute(pt, "name"), mxml_get_attribute(pn, "name"))) {
                     // put routine end line on end stack
                     seq.subroutine_end_line[seq.stack_index] = mxml_get_line_number_end(pt);
                     // go to first line of subroutine
                     seq.current_line_number = mxml_get_line_number_start(pt) + 1;
                     // put parameter(s) on stack
                     if (mxml_get_value(pn)) {
                        char p[256];
                        if (strchr(mxml_get_value(pn), '$')) // evaluate message string if $ present
                           strlcpy(p, eval_var(seq, c, mxml_get_value(pn)).c_str(), sizeof(p));
                        else
                           strlcpy(p, mxml_get_value(pn), sizeof(p)); // treat message as string

                        strlcpy(seq.subroutine_param[seq.stack_index], p, 256);
                     }
                     // increment stack
                     seq.stack_index++;
                     break;
                  }
               }
            }
         }
         if (i == mxml_get_line_number_end(mxml_find_node(c->pnseq, "RunSequence"))) {
            sprintf(str, "Subroutine '%s' not found", mxml_get_attribute(pn, "name"));
            seq_error(seq, c, str);
         }
      }
   }

   /*---- <unknown> ----*/
   else {
      sprintf(str, "Unknown statement \"%s\"", mxml_get_name(pn));
      seq_error(seq, c, str);
   }

   /* get steering parameters, since they might have been changed in between */
   SEQUENCER seq1;
   size = sizeof(seq1);
   db_get_record(c->hDB, hKeySeq, &seq1, &size, 0);
   seq.running = seq1.running;
   seq.finished = seq1.finished;
   seq.paused = seq1.paused;
   seq.debug = seq1.debug;
   seq.stop_after_run = seq1.stop_after_run;
   strlcpy(seq.message, seq1.message, sizeof(seq.message));

   // in debugging mode, pause after each step
   if (seq.debug && !skip_step)
      seq.paused = TRUE;

   /* update current line number */
   db_set_record(c->hDB, hKeySeq, &seq, sizeof(seq), 0);
}

/*------------------------------------------------------------------*/

void init_sequencer(SEQUENCER& seq, SeqCon* c) {
   int status;
   HNDLE hKey;

   c->odbs = gOdb->Chdir(c->odb_path.c_str(), true); // create /Sequencer
   assert(c->odbs);

   status = db_find_key(c->hDB, 0, c->odb_path.c_str(), &c->hSeq);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "init_sequencer", "Sequencer error: Cannot find /Sequencer, db_find_key() status %d", status);
      return;
   }

   status = db_check_record(c->hDB, c->hSeq, "State", strcomb1(sequencer_str).c_str(), TRUE);
   if (status == DB_STRUCT_MISMATCH) {
      cm_msg(MERROR, "init_sequencer", "Sequencer error: mismatching /Sequencer/State structure, db_check_record() status %d", status);
      return;
   }

   status = db_find_key(c->hDB, c->hSeq, "State", &hKey);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "init_sequencer", "Sequencer error: Cannot find /Sequencer/State, db_find_key() status %d", status);
      return;
   }

   int size = sizeof(seq);
   status = db_get_record1(c->hDB, hKey, &seq, &size, 0, strcomb1(sequencer_str).c_str());
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "init_sequencer", "Sequencer error: Cannot get /Sequencer/State, db_get_record1() status %d", status);
      return;
   }

   if (strlen(seq.path) > 0 && seq.path[strlen(seq.path) - 1] != DIR_SEPARATOR) {
      strlcat(seq.path, DIR_SEPARATOR_STR, sizeof(seq.path));
   }

   if (seq.filename[0])
      seq_open_file(seq.filename, seq, c);

   seq.transition_request = FALSE;

   db_set_record(c->hDB, hKey, &seq, sizeof(seq), 0);

   status = db_watch(c->hDB, hKey, seq_watch, c);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "init_sequencer", "Sequencer error: Cannot watch /Sequencer/State, db_watch() status %d", status);
      return;
   }

   bool b = false;
   c->odbs->RB("Command/Start script", &b, true);
   b = false;
   c->odbs->RB("Command/Stop immediately", &b, true);
   b = false;
   c->odbs->RB("Command/Stop after run", &b, true);
   b = false;
   c->odbs->RB("Command/Cancel stop after run", &b, true);
   b = false;
   c->odbs->RB("Command/Pause script", &b, true);
   b = false;
   c->odbs->RB("Command/Resume script", &b, true);
   b = false;
   c->odbs->RB("Command/Debug script", &b, true);
   b = false;
   c->odbs->RB("Command/Step over", &b, true);
   b = false;
   c->odbs->RB("Command/Load new file", &b, true);
   std::string s;
   c->odbs->RS("Command/Load filename", &s, true);

   status = db_find_key(c->hDB, c->hSeq, "Command", &hKey);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "init_sequencer", "Sequencer error: Cannot find /Sequencer/Command, db_find_key() status %d", status);
      return;
   }

   status = db_watch(c->hDB, hKey, seq_watch_command, c);
   if (status != DB_SUCCESS) {
      cm_msg(MERROR, "init_sequencer", "Sequencer error: Cannot watch /Sequencer/Command, db_watch() status %d", status);
      return;
   }
}

/*------------------------------------------------------------------*/

int main(int argc, const char *argv[]) {
   int daemon = FALSE;
   int status, ch;
   char midas_hostname[256];
   char midas_expt[256];
   std::string seq_name = "";

   setbuf(stdout, NULL);
   setbuf(stderr, NULL);
#ifdef SIGPIPE
   /* avoid getting killed by "Broken pipe" signals */
   signal(SIGPIPE, SIG_IGN);
#endif

   /* get default from environment */
   cm_get_environment(midas_hostname, sizeof(midas_hostname), midas_expt, sizeof(midas_expt));

   /* parse command line parameters */
   for (int i = 1; i < argc; i++) {
      if (argv[i][0] == '-' && argv[i][1] == 'D') {
         daemon = TRUE;
      } else if (argv[i][0] == '-') {
         if (i + 1 >= argc || argv[i + 1][0] == '-')
            goto usage;
         if (argv[i][1] == 'h')
            strlcpy(midas_hostname, argv[++i], sizeof(midas_hostname));
         else if (argv[i][1] == 'e')
            strlcpy(midas_expt, argv[++i], sizeof(midas_hostname));
         else if (argv[i][1] == 'c')
            seq_name = argv[++i];
      } else {
         usage:
         printf("usage: %s [-h Hostname[:port]] [-e Experiment] [-c Name] [-D]\n\n", argv[0]);
         printf("       -e experiment to connect to\n");
         printf("       -c Name of additional sequencer, i.e. \'Test\'\n");
         printf("       -h connect to midas server (mserver) on given host\n");
         printf("       -D become a daemon\n");
         return 0;
      }
   }

   if (daemon) {
      printf("Becoming a daemon...\n");
      ss_daemon_init(FALSE);
   }

   std::string prg_name = "Sequencer";
   std::string odb_path = "Sequencer";
   
   if (!seq_name.empty()) {
      prg_name = std::string("Sequencer") + seq_name; // sequencer program name
      odb_path = std::string("Sequencer") + seq_name; // sequencer ODB path
   }

#ifdef OS_LINUX
   std::string pid_filename;
   pid_filename += "/var/run/";
   pid_filename += prg_name;
   pid_filename += ".pid";
   /* write PID file */
   FILE *f = fopen(pid_filename.c_str(), "w");
   if (f != NULL) {
      fprintf(f, "%d", ss_getpid());
      fclose(f);
   }
#endif

   /*---- connect to experiment ----*/
   status = cm_connect_experiment1(midas_hostname, midas_expt, prg_name.c_str(), NULL, DEFAULT_ODB_SIZE, DEFAULT_WATCHDOG_TIMEOUT);
   if (status == CM_WRONG_PASSWORD)
      return 1;
   else if (status == DB_INVALID_HANDLE) {
      std::string s = cm_get_error(status);
      puts(s.c_str());
   } else if (status != CM_SUCCESS) {
      std::string s = cm_get_error(status);
      puts(s.c_str());
      return 1;
   }

   /* check if sequencer already running */
   status = cm_exist(prg_name.c_str(), TRUE);
   if (status == CM_SUCCESS) {
      printf("%s runs already.\n", prg_name.c_str());
      cm_disconnect_experiment();
      return 1;
   }

   HNDLE hDB;
   cm_get_experiment_database(&hDB, NULL);
   gOdb = MakeMidasOdb(hDB);

   SEQUENCER seq;
   SeqCon c;

   c.seqp = &seq;
   c.hDB = hDB;
   c.seq_name = seq_name;
   c.prg_name = prg_name;
   c.odb_path = odb_path;

   init_sequencer(seq, &c);

   if (seq_name.empty()) {
      printf("Sequencer started. Stop with \"!\"\n");
   } else {
      printf("%s started.\n", prg_name.c_str());
      printf("Set ODB String \"/Alias/Sequencer%s\" to URL \"?cmd=sequencer&seq_name=%s\"\n", seq_name.c_str(), seq_name.c_str()); // FIXME: seq_name should be URL-encoded! K.O. Aug 2023
      printf("Stop with \"!\"\n");
   }

   // if any commands are active, process them now
   seq_watch_command(hDB, 0, 0, &c);

   /* initialize ss_getchar */
   ss_getchar(0);

   /* main loop */
   do {
      try {
         sequencer(seq, &c);
      } catch (std::string &msg) {
         seq_error(seq, &c, msg.c_str());
      } catch (const char *msg) {
         seq_error(seq, &c, msg);
      }

      status = cm_yield(0);

      ch = 0;
      while (ss_kbhit()) {
         ch = ss_getchar(0);
         if (ch == -1)
            ch = getchar();

         if ((char) ch == '!')
            break;
      }

   } while (status != RPC_SHUTDOWN && ch != '!');

   /* reset terminal */
   ss_getchar(TRUE);

   /* close network connection to server */
   cm_disconnect_experiment();

   return 0;
}

/*------------------------------------------------------------------*/

/**dox***************************************************************/
/** @} */ /* end of alfunctioncode */

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
