/* rubynokii.c version 0.1.1

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  RubyNokii Copyright (C) 2008 by Thyagarajan Shanmugham
  Gnokii,Libgnokii3 Copyright (C) respective Gnokii developers.. refer www.gnokii.org
  Special thanks to pkot,dforsi and oftopik for their unbounded support.
  
Make sure gnokii works in the command line, then go in for execution of the making of RubyNokii
 
 the extconf.rb file contains

require 'mkmf'
abort 'need ruby.h' unless have_header("ruby.h")
dir_config('rubynokii')
have_library('gnokii')
create_makefile('rubynokii')

[EOF]

 
 
 compile with
 $ruby extconf.rb
 $make
 
 

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <ruby.h>
#include <gnokii.h>

#define MAX_SMS_PART 140

VALUE RubyNokiiModule, RubyNokiiClass;

struct gn_statemachine *state;
int quit = 0;
char *program_name;
static gn_error	error;

static VALUE busterminate(void)
{
	gn_lib_phone_close(state);
	gn_lib_phoneprofile_free(&state);
	gn_lib_library_free();
	return INT2NUM(error);
}

static VALUE businit(void)
{
	error = gn_lib_phoneprofile_load(NULL, &state);
	if (GN_ERR_NONE == error) {
		 error = gn_lib_phone_open(state); 
		if (GN_ERR_NONE == error)
			return INT2NUM(error);
		else
			return INT2NUM(error);
	}
	if (GN_ERR_NONE != error) {
		return INT2NUM(error);
		
	}
        return 0;
}

// refinement of this function needed..into the TODO list 
static VALUE  gn_lib_get_phone_information(void)
{
	gn_data *data = &state->sm_data;
	const char *unknown = _("Unknown");
	gn_error error;
 
	gn_data_clear(data);
	data->model        = state->config.m_model;
	data->manufacturer = state->config.m_manufacturer;
	data->revision     = state->config.m_revision;
	data->imei         = state->config.m_imei;

	error = gn_sm_functions(GN_OP_Identify, data, state);

	if (!data->model[0])
		snprintf(data->model, GN_MODEL_MAX_LENGTH, "%s", unknown);
	if (!data->manufacturer[0])
		snprintf(data->manufacturer, GN_MANUFACTURER_MAX_LENGTH, "%s", unknown);
	if (!data->revision[0])
		snprintf(data->revision, GN_REVISION_MAX_LENGTH, "%s", unknown);
	if (!data->imei[0])
		snprintf(data->imei, GN_IMEI_MAX_LENGTH, "%s", unknown);
	
	printf("\n MODEL = %s \n IMEI = %s \n REVISION = %s\n MANUFACTURER =%s\n",data->model,data->imei,data->revision,data->manufacturer);
		printf("%d", gn_sms_send(data,state));
		return INT2NUM(error);
}



static VALUE sendsms(VALUE self, VALUE  names)
{
	gn_sms sms;
	gn_error error;
	gn_data *data;
	//gn_sms_user_data *udata;
	struct RArray *names_array;
	
	int input_len, i, curpos = 0;
	
	names_array = RARRAY(names);
   
       VALUE current = names_array->ptr[0];
       VALUE mobileno = names_array->ptr[1];

	input_len = GN_SMS_MAX_LENGTH  ;

	gn_sms_default_submit(&sms);

	memset(&sms.remote.number, 0, sizeof(sms.remote.number));
	
	if(rb_respond_to(mobileno, rb_intern("to_s")))
        {
          VALUE receipient_no = rb_funcall(mobileno, rb_intern("to_s"), 0);
	  snprintf(sms.remote.number, 56, "%s",StringValuePtr(receipient_no));
	}

	if (sms.remote.number[0] == '+') 
		sms.remote.type = GN_GSM_NUMBER_International;
	else 
		sms.remote.type = GN_GSM_NUMBER_Unknown;

	sms.dcs.u.general.alphabet = GN_SMS_DCS_DefaultAlphabet;
 	
	if (!sms.smsc.number[0]) {
		data->message_center = calloc(1, sizeof(gn_sms_message_center));
		data->message_center->id = 1;
		
		if (gn_sm_functions(GN_OP_GetSMSCenter, data, state) == GN_ERR_NONE) {
			if (sms.smsc.number[0] == '+') 
		   		sms.smsc.type = GN_GSM_NUMBER_International;
			else
				sms.smsc.type = GN_GSM_NUMBER_Unknown;
			
			snprintf(sms.smsc.number, sizeof(sms.smsc.number), "%s", data->message_center->smsc.number);
			sms.smsc.type = data->message_center->smsc.type;
		} else {
			fprintf(stderr, _("Cannot read the SMSC number from your phone. If the sms send will fail, please use --smsc option explicitely giving the number.\n"));
		}
		free(data->message_center);
	}

	if (!sms.smsc.type) sms.smsc.type = GN_GSM_NUMBER_Unknown;
	
			
	if (curpos != -1) {

	
	sms.user_data[curpos].type = GN_SMS_DATA_Text;
	sms.dcs.u.general.alphabet = GN_SMS_DCS_DefaultAlphabet;
	 if(rb_respond_to(current, rb_intern("to_s")))
       {
           VALUE name = rb_funcall(current, rb_intern("to_s"), 0);
	       	
	 snprintf(sms.user_data[curpos].u.text,255*MAX_SMS_PART,"%s",StringValuePtr(name));;
         sms.user_data[curpos].length =  printf("Message is : %s, ", StringValuePtr(name));
       }
   

	//printf("the length is %d",INT2NUM(strlen(sms.user_data[curpos].u.text)));
		if (sms.user_data[curpos].length < 1) {
			fprintf(stderr, _("Empty message. Quitting.\n"));
			return INT2NUM(GN_ERR_FAILED);
		}
		
		//sms.user_data[curpos].type = GN_SMS_DATA_Text;
		if (!gn_char_def_alphabet(sms.user_data[curpos].u.text))
			sms.dcs.u.general.alphabet = GN_SMS_DCS_UCS2; 
		sms.user_data[++curpos].type = GN_SMS_DATA_None;

	
			


	}
	

	data->sms = &sms;

	/* Send the message. */
	error = gn_sms_send(data, state);

	if (error == GN_ERR_NONE)
		fprintf(stderr, _("Send succeeded!\n"));
	else
		fprintf(stderr, _("SMS Send failed (%s)\n"));

	return INT2NUM(error);
}


void Init_rubynokii()
{
RubyNokiiModule = rb_define_module("RubyNokiiMod");
RubyNokiiClass =  rb_define_class_under(RubyNokiiModule, "BaseChildClass", rb_cObject);
rb_define_method(RubyNokiiClass, "businit", businit, 0);
rb_define_method(RubyNokiiClass,"busterminate",busterminate,0);
rb_define_method(RubyNokiiClass,"gn_lib_get_phone_information",gn_lib_get_phone_information,0);
rb_define_method(RubyNokiiClass,"sendsms",sendsms,1);

}



