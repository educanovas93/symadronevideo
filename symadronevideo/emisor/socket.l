%{
int i;
%}
/* Declaraciones */
D	    [0-9]
L		[a-zA-Z]
imgchars	[^ \n\t\r]
/* Condición de contexto exclusiva */

%x imagen
%%
 /* Reglas */

"--ipcamera\r\nContent-Type: image/jpeg\r\nContent-Length: "{D}+"\r\n\r\n"	{ BEGIN(imagen); i = 0; }
<imagen>.|\n																{ yymore(); i++; }
<imagen>"\r\n"/--ipcamera													{ BEGIN(0); return i; }
.|\n																		{  }

%%
