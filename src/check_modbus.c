/* Original version 0.01 made by Roman Suchkov aka Fineson
 * 
 * 0.3 version created by Andrey Skvortsov 
 */



#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "modbus/modbus.h"
#include <errno.h>
#include "compile_date_time.h"
#include "check_modbus.h"

#define STR(S)                      #S       // STR(blabla) = "blabla"
#define XSTR(S)                     STR(S)   // STR(_version) = "v1.0" if _version = "v1.0"


void print_help(void)
{
    printf("Check ModBus TCP version %s\n", XSTR(PACKAGE_VERSION) );
    printf("Build date: %02d.%02d.%04d\n",COMPILE_DAY,COMPILE_MONTH,COMPILE_YEAR);
    printf("\n");
    printf("-v  --verbose       Print settings before working\n" );
    printf("-h  --help          Print this help\n" );
    printf("-H  --ip=           IP address or hostname\n"); 
    printf("-p  --port=         [ TCP Port number. Default 502 ]\n");  
    printf("-d  --device=       [ Device modbus number. Default 1 ]\n"); 
    printf("-a  --address=      [ Register/bit address reference. Default 1 ]\n"); 
    printf("-t  --try=          [ Number of tries. Default 1 ]\n"); 
    printf("-F  --format=	    [ Data format. Default 1 ]\n"); 
    printf(" 		                1 -  int16_t \n"); 
    printf(" 		                2 - uint16_t \n"); 
    printf(" 		                3 -  int32_t \n");
    printf(" 		                4 - uint32_t \n");
    printf(" 		                5 -  int64_t \n");
    printf(" 		                6 - uint64_t \n");
    printf(" 		                7 -  float   \n");
    printf(" 		                8 -  double  \n");
    printf("-s  --swapbytes     [ Swap bytes in each incomming word ]\n");
    printf("-i  --inverse       [ Use inversed words order ]\n");
    printf("-f  --function=	    Number of functions\n");
    printf(" 		                1 - Read coils\n"); 
    printf(" 		                2 - Read input discretes\n");
    printf(" 		                3 - Read multiple registers\n");
    printf(" 		                4 - Read input registers\n");
    printf("-w  --warning=	    [ Warning range ]\n");
    printf("-c  --critical=	    [ Critical range ]\n");
    printf("-n  --null          [ If the query will get zero, return the critical signal ]\n");
    printf("-N  --not_null	    [ If the query will get no zero, return the critical signal ]\n"); 
    printf("\n");
    printf(" Example: ./check_modbus --ip=192.168.1.123 -d 1 -a 13 -f 4 -w 123.4 -c 234.5\n");
    printf(" Example: ./check_modbus --ip=192.168.1.123 -d 1 -a 15 -f 4 -w 2345 -c 1234\n");
    printf(" Example: ./check_modbus --ip=plc01 --try=5 -d 2 -a 20 -f 2 -n\n");
    printf("\n");
}
 
void print_settings(modbus_params_t* params)
{
    printf("---------------------------------------------\n");
    printf("Settings:\n");
    printf("ip:         %s\n",          params->host        );
    printf("port:       %s\n",          params->mport       );
    printf("device:     %d\n",          params->devnum      );
    printf("address:    %d\n",          params->sad         );
    printf("function:   %d\n",          params->nf          );
    printf("tries:      %d\n",          params->tries       );
    printf("\n");
    printf("inverse:    %d\n",          params->inverse_words);
    printf("format:     %d\n",          params->format      );
    printf("swap bytes: %d\n",          params->swap_bytes  );
    printf("\n");
    printf("warning:    %lf\n",         params->warn_range  );
    printf("critical:   %lf\n",         params->crit_range  );
    printf("null:       %d\n",          params->nc          );
    printf("not null:   %d\n",          params->nnc         );
    printf("---------------------------------------------\n");
}


void    load_defaults(modbus_params_t* params)
{
        static char  mport_default[] = "502";

        params->mport       = mport_default;
        params->sad         = 1;
        params->devnum      = 1;		           
        params->host        = NULL;
        params->nf          = 0;
        params->nc          = 0;
        params->nnc         = 0;
        params->tries       = 1;
        params->format      = 1;
        params->inverse_words  = 0;
        params->swap_bytes  = 0;

        params->warn_range  = 0; 
        params->crit_range  = 0; 
        params->verbose     = 0;
}




int     check_function_num(int fn)
{
    int rc; 
    rc =  (fn>MBF_MIN_SUPPORTED) && (fn<MBF_MAX_SUPPORTED) ? 0 : 1 ;
    return rc;
}


int     check_format_type(int ft)
{
    int rc; 
    rc =  (ft>FORMAT_MIN_SUPPORTED) && (ft<FORMAT_MAX_SUPPORTED) ? 0 : 1 ;
    return rc;
}

int     parse_command_line(modbus_params_t* params, int argc, char **argv)
{
    int rs;
    int option_index;

    const char* short_options = "hH:p:d:a:f:w:c:nNt:F:isv";
    const struct option long_options[] = {
        {"help",no_argument,NULL,'h'},
        {"ip",required_argument,NULL,'H'},
        {"port",optional_argument,NULL,'p'},
        {"device",optional_argument,NULL,'d'},
        {"address",required_argument,NULL,'a'},
        {"try",required_argument,NULL,'t'},
        {"function",required_argument,NULL,'f'},
        {"warning",required_argument,NULL,'w'},
        {"critical",required_argument,NULL,'c'},
        {"null",no_argument,NULL,'n'},
        {"not_null",no_argument,NULL,'N'},
        {"swapbytes",no_argument,NULL,'s'},
        {"inverse",no_argument,NULL,'i'},
        {"verbose",no_argument,NULL,'v'},
        {NULL,0,NULL,0}
        };

    //************************************************************
    if (argc < 2) 
    {
        printf("%s: Could not parse arguments\n", argv[0]);
        print_help(); 
        return RESULT_WRONG_ARG; 
    };

    load_defaults( params );
    while (1) 
    {
	    rs=getopt_long(argc,argv,short_options,long_options,&option_index);
        if (rs == -1) break;

        switch(rs)
        {
            case 'v':
                params->verbose = 1;
                break;
            case 'h':
                print_help(); 
                return RESULT_PRINT_HELP;
            case 'H':
                params->host = optarg;
                break;
            case 'p':
                params->mport = optarg;
                break;
            case 'd':
                params->devnum = atoi(optarg);
                break;
            case 'a':
                params->sad = atoi(optarg);
                params->sad --; /* register/bit address starts from 0 */
                break;
            case 'f':
                params->nf = atoi(optarg);
                break;
            case 'F':
                params->format = atoi(optarg);
                break;
            case 'w':
                params->warn_range = atof(optarg);
                break;
            case 'c':
                params->crit_range = atof(optarg);
                break;
            case 'n':
                params->nc = 1;
                break;
            case 'i':
                params->inverse_words = 1;
                break;
            case 's':
                params->swap_bytes = 1;
                break;
            case 't':
                params->tries = atoi(optarg);
                break;
            case 'N':
                params->nnc = 1;
                break;
            case '?': 
            default:
                print_help(); 
                return RESULT_PRINT_HELP;
        };
    };  /* while(1) */
    
    if (params->host== NULL) 
    {
        printf("Could not parse host address: %s\n", argv[0]);
        return RESULT_WRONG_ARG; 
    };

    if (check_function_num( params->nf ))
    {
        printf("Invalid function number: %d\n", params->nf );
        return RESULT_WRONG_ARG;
    }

    if (check_format_type( params->format) )
    {
        printf("Invalid data format: %d\n", params->format );
        return RESULT_WRONG_ARG;
    }

    if (params->verbose) print_settings( params );


    return RESULT_OK;
}


int     read_data(modbus_t* mb, modbus_params_t* params, data_t*    data)
{
    int rc;
    int size = sizeof_data_t( data );
    int sad  = params->sad;

    clear_data_t( data );
    switch (params->nf)
    {
        case MBF001_READ_COIL_STATUS: 
        	rc = modbus_read_bits(mb, sad, size , data->val.bytes);
            rc =  ((rc == -1) || (rc!=size)) ? RESULT_ERROR_READ : RESULT_OK;
            break;
        case MBF002_READ_INPUT_STATUS: 
            rc = modbus_read_input_bits(mb, sad, size, data->val.bytes);
            rc =  ((rc == -1) || (rc!=size)) ? RESULT_ERROR_READ : RESULT_OK;
            break;
        case MBF003_READ_HOLDING_REGISTERS: 
            rc = modbus_read_registers(mb, sad, size, data->val.words);
            rc =  ((rc == -1) || (rc!=size)) ? RESULT_ERROR_READ : RESULT_OK;
            break;
        case MBF004_READ_INPUT_REGISTERS: 
            rc = modbus_read_input_registers(mb, sad, size, data->val.words);
            rc =  ((rc == -1) || (rc!=size)) ? RESULT_ERROR_READ : RESULT_OK;
            break;
        default:
            rc = RESULT_UNSUPPORTED_FUNCTION;
            break;
    }
    reorder_data_t( data, params->swap_bytes, params->inverse_words );
    return rc;
}





int     print_error( int rc )
{
    switch( rc )
    {
        case RESULT_ERROR_CONNECT: 
            printf("Connection failed: %s\n:", modbus_strerror(errno) );
            break;
        case RESULT_ERROR_READ:
    	    printf("Read failed: %s\n", modbus_strerror(errno) );
            break;
        case RESULT_UNSUPPORTED_FORMAT:
            printf("Invalid data format\n");
            break;
        case RESULT_UNSUPPORTED_FUNCTION:
            printf("Invalid function number\n");
            break;
        case RESULT_OK:
            break;
        default:
            printf("Unsupported return code (%d)\n", rc);
            break;
    }
}


int print_result(modbus_params_t* params, data_t* data)
{
    int rc = RESULT_UNKNOWN;
    double   result, warn_range, crit_range;


    result      = value_data_t(data);
    warn_range  = params->warn_range;
    crit_range  = params->crit_range;

    if (params->nc != params->nnc )  
    {
        if (params->nc  == 1 )  rc = ( result == 0 ) ? RESULT_CRITICAL : RESULT_OK; 
        if (params->nnc == 1 )  rc = ( result != 0 ) ? RESULT_CRITICAL : RESULT_OK; 
	}
    else
    {
        if ( warn_range <= crit_range) 
        {
            if ( result >= crit_range)      rc = RESULT_CRITICAL;
            else rc = ( result >= warn_range) ? RESULT_WARNING : RESULT_OK;
        }
        else
        {
            if ( result <= crit_range)      rc = RESULT_CRITICAL;
            else rc = ( result <= warn_range) ?  RESULT_WARNING : RESULT_OK;
        }
    }


    switch(rc)
    {
        case RESULT_OK:
            printf("Ok: ");
            break;
        case RESULT_WARNING:
            printf("Warning: ");
            break;
        case RESULT_CRITICAL:
            printf("Critical: ");
            break;
        case RESULT_UNKNOWN:
            printf("Unknown result");
            break;
    }

    printf_data_t( data ); 
    printf("\n"); 

    return rc;
}


int     process(modbus_params_t* params )
{
        modbus_t        *mb;
        const int       retry_max_timeout_us    =   100*1000; /* us */ 
        int             retry_timeout_us;
        struct timeval  response_timeout;
        int             try_cnt;
        data_t          data;
        int             rc;

        mb = modbus_new_tcp_pi(params->host, params->mport);
        if (mb == NULL) 
        {
            printf( "Unable to allocate libmodbus context\n");
            return RESULT_ERROR;
        }

	    /* set short timeout */
        response_timeout.tv_sec = 1;
        response_timeout.tv_usec = 0;
        modbus_set_response_timeout( mb, &response_timeout );

        /* calculate retry timeout : random from 1.0 to 1.3 of base timeout */
        retry_timeout_us = (1 + (rand() % 30)/100.0 )* ( retry_max_timeout_us*(try_cnt+1) );
        
        modbus_set_slave(mb,params->devnum);

        init_data_t( &data, params->format );
        for(try_cnt=0; try_cnt<params->tries; try_cnt++)
        {
            /* start new try */
            rc = RESULT_OK;

            if (modbus_connect(mb) == -1) 
            {
                usleep( retry_timeout_us );
                rc = RESULT_ERROR_CONNECT;
                continue;
            }

            rc = read_data( mb, params, &data);
            if (rc != RESULT_OK )
            {
	            modbus_close(mb);

                usleep( retry_timeout_us );
                continue;
            }
            else break;

        }

        print_error( rc ); 

        if (rc==RESULT_OK) rc = print_result( params, &data );
        
	    modbus_close(mb);
    	modbus_free(mb);
        return rc;
}




int main(int argc, char **argv)

{
    modbus_params_t  params;
    int                     rc;

    srand( time(NULL) );
    rc = parse_command_line(&params, argc, argv );
    return  ( rc != RESULT_OK ) ? rc : process( &params );
}

