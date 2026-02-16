   case 1: /* Operator found */
      switch (lachar)
      {
      case '[':
      case '{':
	 lachar = '(';
      case '(':
	 token = lachar;
	 break;

      case ']':
      case '}':
	 lachar = ')';
      case ')':
	 token = lachar;
	 break;

      default:
	 if (latran == 1)
	    token = EOS;
	 else
	    token = lachar;
      }
      needop = 0;
      break;

   case 2: /* Symbol found */
      value = 0;
      token = SYM;
      needop = 1;
      break;

   case 3: /* Decimal number found */
      value = value * 10 + dignum;
      token = DECNUM;
      needop = 1;
      break;

   case 4: /* PC symbol found */
      value = pc;
      token = PC;
      needop = 1;
      if (indseg)
	   segvaltype = DSEGSYM;
      else if (incseg)
	   segvaltype = CSEGSYM;
      relocatable++;
      break;

   case 5: /* Hex Number or gt relop */
      if (!strcmp (symbol, ">=")) token = GE;
      else token = GT;
      needop = 0;
      break;

   case 6: /* Hex Number found */
      if (needop) /* If true, then we need an operator */
      {
	 if (!strcmp (symbol, ">")) token = GT;
	 else token = GE;
	 *ndx = *ndx - 1;
	 value = 0;
	 next = 0;
	 needop = 0;
	 break;
      }
      value = (value << 4) + dignum;
      token = HEXNUM;
      symbol[0] = '\0';
      if (!isxdigit (lachar))
	 needop = 1;
      break;

   case 7: /* operator found */
      needop = 0;
      if (!strcmp (symbol, "=")) token = EQ;
      else if (!strcmp (symbol, "#=")) token = NE;
      else if (!strcmp (symbol, "#")) token = NOT;
      else if (!strcmp (symbol, "<=")) token = LE;
      else if (!strcmp (symbol, "<")) token = LT;
      else if (!strcmp (symbol, ">=")) token = GE;
      else if (!strcmp (symbol, ">")) token = GT;
      else if (!strcmp (symbol, "&")) token = AND;
      else if (!strcmp (symbol, "+"))
      {
	 *term = symbol[0];
	 token = symbol[0];
	 symbol[0] = '\0';
      }
      else if (!strcmp (symbol, "++")) token = OR;
      value = 0;
      break;

   case 9: /* String found */
      {
	 int i, len;
	 len = strlen(symbol);
	 for (i = 0; i < len; i++)
	    symbol[i] = TOASCII (symbol[i]);
	 if (len <= sizeof (tokval))
	    for (i = 0; i < len; i++)
	       value = (value << 8) | symbol[i];
	 token = STRING;
      }
      needop = 1;
      break;

   case 10: /* Divide or right shift op found */
      if (!strcmp (symbol, "//")) token = RTSHIFT;
      else token = '/';
      needop = 0;
      break;

   case 11: /* Comma found, end of expression */
      token = EOS;
      needop = 0;
      break;

   case 13: /* recognize exponent sign*/
      token = FLTNUM;
      needop = 1;
      break;

   case 14: /* recognize exponent */
      token = FLTNUM;
      sexp = sexp * 10 + dignum;
      needop = 1;
      break;

   case 15: /* process exponent sign */
      token = FLTNUM;
      if (lachar == '-')
	 expsgn = -1;
      needop = 1;
      break;

   case 16: /* process fraction */
      token = FLTNUM;
      if (dignum == 0)
      {
	 if (!digitfound)
	    leadingzero++;
      }
      else digitfound = TRUE;
      fracdigits++;
      sfrc = sfrc * 10 + dignum;
      needop = 1;
      break;

   case 17: /* build exponential form */
      {
	 union {
	    DOUBLE xxd;
	    uint32 xxl[2];
	 } xx;
	 char *last;

	 token = FLTNUM;
#ifdef DEBUG_STRTOD
	 printf ("Float format: \n");
	 printf ("   before symbol = %s\n", symbol);
#endif
	 xx.xxd = ibm_strtod (symbol, &last);
#ifdef ASM_BIG_ENDIAN
	 sprintf (symbol, "%8.8X%8.8X", xx.xxl[0], xx.xxl[1]);
#else
	 sprintf (symbol, "%8.8X%8.8X", xx.xxl[1], xx.xxl[0]);
#endif
#ifdef DEBUG_STRTOD
	 printf ("   symbol = %x, last = %x\n",
		  (long)symbol, (long)last);
	 printf ("   after symbol = %s\n", symbol);
#endif
      }
      needop = 1;
      break;

   case 18: /* Number found */
      evalue = evalue * 10 + dignum;
      numdigits++;
      token = FLTNUM;
      needop = 1;
      break;

   case 19: /* process sign */
      token = FLTNUM;
      if (lachar == '-')
	 numsgn = -1;
      needop = 1;
      break;

   case 21: /* process macvar */
      token = MACSYM;
      needop = 1;
      break;
