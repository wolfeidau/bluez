%{
/* 
   BlueZ - Bluetooth protocol stack for Linux
   Copyright (C) 2000-2001 Qualcomm Incorporated
   
   Written 2000,2001 by Maxim Krasnyansky <maxk@qualcomm.com>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY CLAIM,
   OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
   RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
   NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
   USE OR PERFORMANCE OF THIS SOFTWARE.
   
   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, COPYRIGHTS,
   TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS SOFTWARE IS DISCLAIMED.
*/

/*
 * $Id$
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>

#include <sys/socket.h>
#include <asm/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "hcid.h"
#include "kword.h"

int cfg_error(const char *fmt, ...);

int yyparse(void);
int yylex(void);	
int yyerror(char *s); 

%}

%union {
	char *str;
	long  num;
}

%token K_OPTIONS K_DEVICE
%token K_AUTOINIT K_SECURITY K_PAIRING
%token K_PTYPE K_NAME K_CLASS K_LM K_LP K_AUTH K_ENCRYPT K_ISCAN K_PSCAN
%token K_PINHELP
%token K_YES K_NO

%token <str> WORD PATH STRING LIST
%token <num> NUM

%type  <num> bool pkt_type link_mode link_policy sec_mode pair_mode
%type  <str> dev_name

%%
config: statement | config statement;
statement: 
  K_OPTIONS hcid_options
 
  | K_DEVICE device_options

  | WORD	{
			cfg_error("Invalid statement '%s'", $1);
		}
  | error	{
			yyclearin; yyerrok;
		}
  ;

hcid_options: '{' hcid_opts '}'
hcid_opts: | hcid_opt ';' | error ';' | hcid_opts hcid_opt ';';
hcid_opt: 
  K_AUTOINIT bool	{
				hcid.auto_init = $2;
			}

  | K_SECURITY sec_mode	{
				hcid.security = $2;
			}

  | K_PAIRING pair_mode	{
				hcid.pairing = $2;
			}

  | K_PINHELP  PATH	{
				if (hcid.pin_helper)
					free(hcid.pin_helper);
				hcid.pin_helper = strdup($2);
			}

  | WORD		{
				cfg_error("Unknown option '%s'", $1);
			}
  ;

sec_mode:
   WORD		{	
			int opt = find_keyword(sec_param, $1);
			if (opt < 0) {
				cfg_error("Unknown security mode '%s'", $1);
				$$ = 0;
			} else
				$$ = opt;
		}

  | K_NO	{ 	$$ = HCID_SEC_NONE; 	}
  ;

pair_mode:
   WORD		{	
			int opt = find_keyword(pair_param, $1);
			if (opt < 0) {
				cfg_error("Unknown pairing mode '%s'", $1);
				$$ = 0;
			} else
				$$ = opt;
		}
  ;


device_options: '{' device_opts '}'
device_opts: | device_opt ';' | error ';' | device_opts device_opt ';';
device_opt:
  K_PTYPE pkt_type	{
				devi.pkt_type = $2;
			}

  | K_LM link_mode	{
				devi.link_mode = $2;
			}

  | K_LP link_policy	{
				devi.link_policy = $2;
			}

  | K_NAME dev_name	{  
				if (devi.name)
					free(devi.name);
				devi.name = $2;
			}

  | K_CLASS NUM		{
				devi.class = $2;
			}

  | K_AUTH bool		{
				devi.auth = $2;
			}

  | K_ENCRYPT bool	{
				devi.encrypt = $2;
			}

  | K_ISCAN bool	{
				if ($2)
					devi.scan |=  SCAN_INQUIRY;
				else
					devi.scan &= ~SCAN_INQUIRY;
			}

  | K_PSCAN bool	{
				if ($2)
					devi.scan |=  SCAN_PAGE;
				else
					devi.scan &= ~SCAN_PAGE;
			}

  | WORD		{
				cfg_error("Unknown option '%s'",$1);
				YYABORT;
			}
  ;

dev_name:
  WORD		{  
			$$ = strdup($1);
		}

  |  STRING	{
			$$ = strdup($1);
		}
  ;

pkt_type:
  WORD 	 	{
			int opt;
			if (!hci_strtoptype($1, &opt))
				cfg_error("Unknown packet type '%s'", $1);
			$$ = opt;
		}

  | LIST	{
			int opt;
			if (!hci_strtoptype($1, &opt))
				cfg_error("Unknown packet type '%s'", $1);
			$$ = opt;
  		}
  ;

link_mode:
  WORD 	 	{
			int opt;
			if (!hci_strtolm($1, &opt))
				cfg_error("Unknown link mode '%s'", $1);
			$$ = opt;
		}

  | LIST	{
			int opt;
			if (!hci_strtolm($1, &opt))
				cfg_error("Unknown link mode '%s'", $1);
			$$ = opt;
  		}
  ;

link_policy:
  WORD 	 	{
			int opt;
			if (!hci_strtolp($1, &opt))
				cfg_error("Unknown link policy '%s'", $1);
			$$ = opt;
		}

  | LIST	{
			int opt;
			if (!hci_strtolp($1, &opt))
				cfg_error("Unknown link policy '%s'", $1);
			$$ = opt;
  		}
  ;

bool: K_YES { $$ = 1; } | K_NO  { $$ = 0; };

%%

int yyerror(char *s) 
{
	syslog(LOG_ERR, "%s line %d\n", s, lineno);
	return 0;
}

int cfg_error(const char *fmt, ...)
{
	char buf[255];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf,sizeof(buf),fmt,ap);
	va_end(ap);

	yyerror(buf);
	return 0;
}

/* 
 * Read config file. 
 */ 
int read_config(char *file) 
{
	extern FILE *yyin;

	if( !(yyin = fopen(file,"r")) ){
		syslog(LOG_ERR,"Can not open %s", file);
		return -1;      
	}

	lineno = 1;
	yyparse();

	fclose(yyin);
  
	return 0;     
}
