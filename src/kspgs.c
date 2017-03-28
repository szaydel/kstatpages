/*
 * name: kspgs.c
 * description: CSV-formatted output equivalent to data produced with
 * 				# kstat -p unix:0:system_pages
 * CC=/opt/gcc-5.1.0/bin gcc -m64 -o kspgs -Wall kspgs.c -lkstat
 */

#include <kstat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysinfo.h>

long getSwapKstatInteger(kstat_ctl_t *, const char *, const char *, char *);
long getKstatInteger(kstat_ctl_t *, const char *, const char *, char *);
char *getKStatString(kstat_ctl_t *, const char *, const char *, char *);

const int DEFLOOPS = 10 ; /* By default run for 10 seconds */
const char * VMINFO = "vminfo" ;
const char * SYSP = "system_pages" ;
const char * MODULE = "unix" ;

/* Fetch swap-specific numerical statistic from kernel */
long getSwapKstatInteger(kstat_ctl_t *kernelDesc, const char *moduleName, 
     const char *recordName, char *fieldName) {
  kstat_t *ks;
  vminfo_t vm;
  if (strcmp(VMINFO, recordName) != 0) return(-1) ; /* This should be vminfo */
  if ((ks = kstat_lookup(kernelDesc, (char *)moduleName, -1, (char *)recordName)) ==
       NULL) {
     return(-1);
  }

  if (kstat_read(kernelDesc, ks, NULL) < 0)
    return(-1);

  if (ks->ks_type == 0 && strcmp(ks->ks_name, VMINFO) == 0) {
    ks = kstat_lookup(kernelDesc, (char *)moduleName, 0, (char *)recordName);
    if (ks) {
      kstat_read(kernelDesc, ks, &vm);

      if (strcmp("swap_resv", fieldName) == 0) {
        return vm.swap_resv ;
      } else if (strcmp("swap_alloc", fieldName) == 0) {
        return vm.swap_alloc ;
      } else if (strcmp("swap_avail", fieldName) == 0) {
        return vm.swap_avail ;
      } else if (strcmp("swap_free", fieldName) == 0) {
        return vm.swap_free ;
      } else {
        return(-1);
      }
    }
  }
  return(-1);
}

/* Fetch numerical statistic from kernel */
long getKstatInteger(kstat_ctl_t *kernelDesc, const char *moduleName, 
     const char *recordName, char *fieldName) {
  kstat_t *ks;
  kstat_named_t *kstatFields;
  long value;
  int i;
       
  if ((ks = kstat_lookup(kernelDesc, (char *)moduleName, -1, (char *)recordName)) ==
       NULL) {
     return(-1);
  }

  if (kstat_read(kernelDesc, ks, NULL) < 0)
    return(-1);

  kstatFields = KSTAT_NAMED_PTR(ks);

  for (i=0; i<ks->ks_ndata; i++) {
    if (strcmp(kstatFields[i].name, fieldName) == 0) {
       switch(kstatFields[i].data_type) {
          case KSTAT_DATA_INT32:
               value = kstatFields[i].value.i32;
               break;
          case KSTAT_DATA_UINT32:
               value = kstatFields[i].value.ui32;
               break;
          case KSTAT_DATA_INT64:
               value = kstatFields[i].value.i64;
               break;
          case KSTAT_DATA_UINT64:
               value = kstatFields[i].value.ui64;
               break;
          default:
               value = -1;
       }
       return(value);
    }
  }
  return(-1);
}

/* Fetch string statistic from kernel */
char *getKStatString(kstat_ctl_t *kernelDesc, const char *moduleName, 
     const char *recordName, char *fieldName) {
  kstat_t *ks;
  kstat_named_t *kstatFields;
  char *value;
  int i;
       
  if ((ks = kstat_lookup(kernelDesc, (char *)moduleName, -1, (char *)recordName)) ==
       NULL) {
     return(NULL);
  }

  if (kstat_read(kernelDesc, ks, NULL) < 0)
    return(NULL);

  kstatFields = KSTAT_NAMED_PTR(ks);

  for (i=0; i<ks->ks_ndata; i++) {
    if (strcmp(kstatFields[i].name, fieldName) == 0) {
       switch(kstatFields[i].data_type) {
          case KSTAT_DATA_CHAR:
               value = kstatFields[i].value.c;
               break;
          default:
               value = NULL;
       }
       return(value);
    }
  }
  return(NULL);
}

int main(int argc, char *argv[]) {
  kstat_ctl_t *kernelDesc;
  int loops;
  int secs = DEFLOOPS;
  long econtig = 0;
  long kernelbase = 0;

 if ((argc > 1) && (strlen(argv[1]) > 0)) {
	 secs = atoi(argv[1]);
	 /* If we did not extract value from command line arg, reset to default. */
	 if (secs == 0) secs = DEFLOOPS;
 }

  /* Open the kernel statistics device */
  if ((kernelDesc = kstat_open()) == NULL) {
     perror("kstat_open");
     return (1);
  }

  /* Prime the pump first and then update these values in the loop. */
  unsigned long swap_alloc_prev = 
    getSwapKstatInteger(kernelDesc, MODULE, VMINFO, "swap_alloc");
  unsigned long swap_avail_prev = 
    getSwapKstatInteger(kernelDesc, MODULE, VMINFO, "swap_avail");
  unsigned long swap_free_prev = 
    getSwapKstatInteger(kernelDesc, MODULE, VMINFO, "swap_free");
  unsigned long swap_resv_prev = 
    getSwapKstatInteger(kernelDesc, MODULE, VMINFO, "swap_resv");

  sleep(1) ; /* Wait for 1 second to let counters update */

  /* Fetch CPU info */
  /*
  printf("CPU Type=%s\n", 
     getKStatString(kernelDesc, "cpu_info", "cpu_info0", "cpu_type"));
  
  value = getKstatInteger(kernelDesc, "cpu_info", "cpu_info0", "clock_MHz");
  printf("CPU Speed=%ld\n", value);

  value = getKstatInteger(kernelDesc, MODULE, "system_misc", "ncpus");
  printf("Number of cpus=%ld\n", value);

  value = getKstatInteger(kernelDesc, MODULE, SYSP, "physmem");
  printf("Number of cpus=%ld\n", value);
  */
  printf("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
		"timestamp","availrmem","desfree","desscan","econtig","fastscan",
	  	"freemem","kernelbase","lotsfree","minfree","nalloc","nalloc_calls",
		"nfree","nfree_calls","nscan","pagesfree","pageslocked","pagestotal",
		"physmem","pp_kernel","slowscan",
    "swap_alloc","swap_avail","swap_free","swap_resv");
  /* Loop for DEFLOOPS times, or number of times specified via command l_line
  argument, print one line output and return. */
  for (loops=0; loops < secs; loops++) {
    /* Fetch the current load averages
    long av1min = getKstatInteger(kernelDesc, MODULE, "system_misc", "avenrun_1min");
	long av5min  = getKstatInteger(kernelDesc, MODULE, "system_misc", "avenrun_5min");
	long av15min  = getKstatInteger(kernelDesc, MODULE, "system_misc", "avenrun_15min");
	*/
  
    /* Fetch information on physical memory usage */
 	long availrmem = getKstatInteger(kernelDesc, MODULE, SYSP, "availrmem");
	long desfree = getKstatInteger(kernelDesc, MODULE, SYSP, "desfree");
	long desscan = getKstatInteger(kernelDesc, MODULE, SYSP, "desscan");
	econtig = getKstatInteger(kernelDesc, MODULE, SYSP, "econtig");
	long fastscan = getKstatInteger(kernelDesc, MODULE, SYSP, "fastscan");
	long freemem = getKstatInteger(kernelDesc, MODULE, SYSP, "freemem");
	kernelbase = getKstatInteger(kernelDesc, MODULE, SYSP, "kernelbase");
	long lotsfree = getKstatInteger(kernelDesc, MODULE, SYSP, "lotsfree");
	long minfree = getKstatInteger(kernelDesc, MODULE, SYSP, "minfree");
	long nalloc = getKstatInteger(kernelDesc, MODULE, SYSP, "nalloc");
	long nalloc_calls = getKstatInteger(kernelDesc, MODULE, SYSP, "nalloc_calls");
	long nfree = getKstatInteger(kernelDesc, MODULE, SYSP, "nfree");
	long nfree_calls = getKstatInteger(kernelDesc, MODULE, SYSP, "nfree_calls");
	long nscan = getKstatInteger(kernelDesc, MODULE, SYSP, "nscan");
	long pagesfree = getKstatInteger(kernelDesc, MODULE, SYSP, "pagesfree");
	long pageslocked = getKstatInteger(kernelDesc, MODULE, SYSP, "pageslocked");
	long pagestotal = getKstatInteger(kernelDesc, MODULE, SYSP, "pagestotal");
	long physmem = getKstatInteger(kernelDesc, MODULE, SYSP, "physmem");
	long pp_kernel = getKstatInteger(kernelDesc, MODULE, SYSP, "pp_kernel");
	long slowscan = getKstatInteger(kernelDesc, MODULE, SYSP, "slowscan");

  /* Fetch information on swapfs usage */
	unsigned long swap_alloc = getSwapKstatInteger(kernelDesc, MODULE, VMINFO, "swap_alloc");
  unsigned long swap_avail = getSwapKstatInteger(kernelDesc, MODULE, VMINFO, "swap_avail");
  unsigned long swap_free = getSwapKstatInteger(kernelDesc, MODULE, VMINFO, "swap_free");
  unsigned long swap_resv = getSwapKstatInteger(kernelDesc, MODULE, VMINFO, "swap_resv");

  /* 
   * We need to use delta from last sample to current sample to properly express
   * number of pages allocated, available, free and reserved.
   */
  unsigned long swap_alloc_delta = swap_alloc - swap_alloc_prev ;
  unsigned long swap_avail_delta = swap_avail - swap_avail_prev ;
  unsigned long swap_free_delta = swap_free - swap_free_prev ;
  unsigned long swap_resv_delta = swap_resv - swap_resv_prev ;

  /* Update previous measurement with current data */
  swap_alloc_prev = swap_alloc ;
  swap_avail_prev = swap_avail ;
  swap_free_prev = swap_free ;
  swap_resv_prev = swap_resv ; 

	time_t ts = time(NULL);
	printf("%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
			ts, availrmem, desfree, desscan, econtig, fastscan, freemem, kernelbase,
			lotsfree, minfree, nalloc, nalloc_calls, nfree, nfree_calls, nscan,
			pagesfree, pageslocked, pagestotal, physmem, pp_kernel, slowscan,
      swap_alloc_delta, swap_avail_delta, swap_free_delta, swap_resv_delta
	);
	/* Per second statistics */
    sleep(1);
  }
  return 0;
}