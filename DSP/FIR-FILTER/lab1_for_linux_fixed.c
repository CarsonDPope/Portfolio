/* CARSON POPE FINAL CODE FOR LAB */
/** @file lab1_for_linux_fixed.c
 *
 * @brief This client reads a .txt file for an FIR filter coefficients. 
 * 			It then convolutes them with the input signal to create a filter.
 *			It also has the option to toggle a pass through signal.
 *
 * This is a modified version of the original simple_client.c program.
 * The original was downloaded on 2012-01-14 from:
 *   http://trac.jackaudio.org/wiki/WalkThrough/Dev/SimpleAudioClient
 *   http://trac.jackaudio.org/browser/trunk/jack/example-clients/simple_client.c
 *   (previous versions/downloads: Oct. 2009)
 *
 * This program initializes the sound card and processes the audio samples from the input to the output.
 * It is modified to create a filter or other DSP application which uses the sound card.
 * For most applications, if used as a template, the only changes needed are in the marked areas;
 * the rest of the program should not need to be modified.
 * Certain lines will be marked with "//-----". This means the referenced line was not added but
 * instead replaced a line of the original code.
 * This version has stereo output. The input is on the left channel and output is directed to 
 * left channel and right channel respectively, depending on the sound card driver configuration.
 *
 * On Linux using gcc compile with the '-ljack' option to link with the jack library.
 * If you use math functions, e.g. cos, the '-lm' option is also needed to link with the math library.
 *   E.g., gcc -Wall myprog.c -ljack -lm -o myprog
 * The -Wall enables most warnings and the -o specifies the executable file name.
 * The order matters on some systems: place the link library list (-l...) after the source file name.
 *
 * Updated: Feb. 2023
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <jack/jack.h>

#define MAX_COEFF 50
#define MAX_LEN 100

jack_port_t *input_port;
jack_port_t *output_port_filtered; //----
jack_port_t *output_port_pass;     //----
jack_client_t *client;

/***** GLOBAL VARIABLE DECLARATIONS HERE *****/
	static float h[MAX_COEFF];
	static int NUM_COEFF;


	bool passthrough = 0;

/**
* This function checks for any input and compares it to the passed character.
* If it matches it will flip the passthrough boolean
**/
void checkInput(int ch){
	if(getchar() == ch){  //the getchar() function in the if statement will wait until a newline
		passthrough = !passthrough;			//before checking if it matches
		fprintf(stderr,"\nPASSTHROUGH %s . . .   \n", passthrough?"ON":"OFF"); // "ON" if passthrough is a 1, "OFF" if passthrough is a 0
		fprintf(stderr,"Please enter a lower case \"f\" to toggle:   ");
	}
}

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * It will exit when stopped by 
 * the user (e.g. using Ctrl-C on a unix-ish operating system)
 */
int process_samples (jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *in;
	jack_default_audio_sample_t *out_filter; //----
	jack_default_audio_sample_t *out_pass;   //----
	int i;
	float x;
	float y;

/***** LOCAL VARIABLE DECLARATIONS HERE *****/
	//----------------------------------------------
		static float delay_line[MAX_COEFF] = { 0 }; 

		int j;
	//----------------------------------------------
	
	in = jack_port_get_buffer (input_port, nframes);
	out_filter = jack_port_get_buffer (output_port_filtered, nframes); //----
	out_pass = jack_port_get_buffer (output_port_pass, nframes);  //----

	for (i=0;i<(int)nframes;i++) {
		x=in[i];

/***** MY ALGORITHM *****/

		delay_line[0] = x;   
		y = 0; //---- Reset Y

		for(j=0;j<NUM_COEFF;j++){
			y += h[j] * delay_line[j]; // convolution loop
		}
			
		for(j=NUM_COEFF-1;j>0;j--){
			delay_line[j]=delay_line[j-1];  // shift loop
		}

		out_filter[i] = y; //----- output the sum

		if(passthrough == 1){
			out_pass[i] = x;   //----- passthrough on
		} else {
			out_pass[i] = 0;   //----- passthrough off
		}

	}

	return 0;      
}


/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown (void *arg)
{
	exit (EXIT_FAILURE);
}


int main (int argc, char *argv[])
{
	const char **ports;
	const char *client_name = "simple-client"; 
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;

        int sample_rate;

/***** LOCAL VARIABLE DECLARATIONS HERE *****/
		FILE * coeff_file;
		char line[MAX_LEN], *firstLine;

	/* open a client connection to the JACK server */

	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (EXIT_FAILURE);
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process_samples()' whenever
	 * there is work to be done.
	 */

	jack_set_process_callback (client, process_samples, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	 * it ever shuts down, either entirely, or if it
	 * just decides to stop calling us.
	 */

	jack_on_shutdown (client, jack_shutdown, 0);

	/* get current sample rate */

        sample_rate = jack_get_sample_rate (client);

        printf ("engine sample rate: %d\n", sample_rate);

	/* create two ports */

	input_port = jack_port_register (client, "input",
					 JACK_DEFAULT_AUDIO_TYPE,
					 JackPortIsInput, 0);
	output_port_filtered = jack_port_register (client, "FilterOutput", //----
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);
	output_port_pass = jack_port_register (client, "PassThroughOutput", //----
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);


	if ((input_port == NULL) || (output_port_filtered == NULL) || (output_port_pass == NULL)){ //-----
		fprintf(stderr, "no more JACK ports available\n");
		exit (EXIT_FAILURE);
	}

/***** INSERT INITIALIZATION CODE HERE *****/
	/***** THIS STATEMENT OPENS THE FILE AND GRACEFULLY EXITS IF IT CANT OPEN ******/
	/***** COMMENTED OUT LINES ALLOW EASY SWITCH BETWEEN INPUT FILES		  ******/

	if((coeff_file = fopen("pope_fir_coefficients_rev5.txt","r")) == NULL){
//	if((coeff_file = fopen("filter_test_TTU_morse_code_h.txt","r")) == NULL){
//	if((coeff_file = fopen("filter_test_HUGE_h.txt","r")) == NULL){	
		fprintf(stderr, "cannot open file\n");
		exit(EXIT_FAILURE);
	}

	//GET FIRST LINE AND PRINT TO COMMAND LINE
	if((firstLine = fgets(line,MAX_LEN,coeff_file)) != NULL){
		printf("File Opened\n");
	}

	//LOOP THROUGH AND SAVE COEFFICIENTS IN h array
	NUM_COEFF = 0;
	while(fscanf(coeff_file,"%f",&h[NUM_COEFF]) != EOF){
		NUM_COEFF++;
		if(NUM_COEFF >= MAX_COEFF-1){
			fprintf(stderr,"maximum coefficients reached\n");
			exit (EXIT_FAILURE);
		}
	}
	
	//	printf("%i\n",NUM_COEFF);                  THIS CAN BE USED FOR
	//	printf("%f %f\n",h[0],h[NUM_COEFF-1]);			DEBUGGING
	
	fclose(coeff_file);

	/* Tell the JACK server that we are ready to begin.
	 * Our process_samples() callback will start running now.
	 */
	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		exit (EXIT_FAILURE);
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */
	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsOutput);
	if (ports == NULL) {
		fprintf(stderr, "no physical capture ports\n");
		exit (EXIT_FAILURE);
	}

	if (jack_connect (client, ports[0], jack_port_name (input_port))) {
		fprintf (stderr, "cannot connect input ports\n");
	}

	free (ports);
	
	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit (EXIT_FAILURE);
	}

	if(jack_connect (client, jack_port_name (output_port_filtered), ports[0])){ //----
		fprintf (stderr, "cannot connect output ports\n");
	}
	if(jack_connect (client, jack_port_name (output_port_pass), ports[1])){ //----
		fprintf (stderr, "cannot connect output ports\n");
	}

	free (ports);

	/* it is now running....*/

/***** ACTIONS TO DO WHILE ALGORITHM IS RUNNING *****/
	//Start up messages
	fprintf(stderr,"Enter f to toggle passthrough ...  \n");
	fprintf(stderr,"Running ... press CTRL-C to exit ...   ");
	while (1) {
		//constantly check for new inputs in the console.
		checkInput('f'); //-----
	}

	/* This is may or may not be reached, depending on what is directly before it,
	 * but if the program had some other way to exit besides being killed,
	 * they would be important to call.
	 */

	jack_client_close (client);
	exit (EXIT_SUCCESS);
}
