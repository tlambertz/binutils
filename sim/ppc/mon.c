/*  This file is part of the program psim.

    Copyright (C) 1994-1995, Andrew Cagney <cagney@highland.com.au>

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
    */


#ifndef _MON_C_
#define _MON_C_

#ifndef STATIC_INLINE_MON
#define STATIC_INLINE_MON STATIC_INLINE
#endif

#include "basics.h"
#include "cpu.h"
#include "mon.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

struct _cpu_mon {
  unsigned issue_count[nr_itable_entries];
  unsigned read_count;
  unsigned write_count;
};

struct _mon {
  int nr_cpus;
  cpu_mon cpu_monitor[MAX_NR_PROCESSORS];
};


INLINE_MON mon *
mon_create(void)
{
  mon *monitor = ZALLOC(mon);
  return monitor;
}


INLINE_MON cpu_mon *
mon_cpu(mon *monitor,
	int cpu_nr)
{
  if (cpu_nr < 0 || cpu_nr >= MAX_NR_PROCESSORS)
    error("mon_cpu() - invalid cpu number\n");
  return &monitor->cpu_monitor[cpu_nr];
}


INLINE_MON void
mon_init(mon *monitor,
	 int nr_cpus)
{
  bzero(monitor, sizeof(*monitor));
  monitor->nr_cpus = nr_cpus;
}


INLINE_MON void
mon_issue(itable_index index,
	  cpu *processor, 
	  unsigned_word cia)
{
  cpu_mon *monitor = cpu_monitor(processor);
  ASSERT(index <= nr_itable_entries);
  monitor->issue_count[index] += 1;
}


INLINE_MON void
mon_read(unsigned_word ea,
	 unsigned_word ra,
	 unsigned nr_bytes,
	 cpu *processor,
	 unsigned_word cia)
{
  cpu_mon *monitor = cpu_monitor(processor);
  monitor->read_count += 1;
}


INLINE_MON void
mon_write(unsigned_word ea,
	  unsigned_word ra,
	  unsigned nr_bytes,
	  cpu *processor,
	  unsigned_word cia)
{
  cpu_mon *monitor = cpu_monitor(processor);
  monitor->write_count += 1;
}


STATIC_INLINE_MON unsigned
mon_get_number_of_insns(cpu_mon *monitor)
{
  itable_index index;
  unsigned total_insns = 0;
  for (index = 0; index < nr_itable_entries; index++)
    total_insns += monitor->issue_count[index];
  return total_insns;
}

STATIC_INLINE_MON char *
mon_add_commas(char *buf,
	       int sizeof_buf,
	       long value)
{
  int comma = 3;
  char *endbuf = buf + sizeof_buf - 1;

  *--endbuf = '\0';
  do {
    if (comma-- == 0)
      {
	*--endbuf = ',';
	comma = 2;
      }

    *--endbuf = (value % 10) + '0';
  } while ((value /= 10) != 0);

  ASSERT(endbuf >= buf);
  return endbuf;
}


INLINE_MON void
mon_print_info(mon *monitor,
	       int verbose)
{
  char buffer[20];
  int cpu_nr;
  int len_cpu;
  int len_num = 0;
  int len;
  long total_insns = 0;
  long cpu_insns_second;
  double cpu_time = 0.0;

  for (cpu_nr = 0; cpu_nr < monitor->nr_cpus; cpu_nr++) {
    unsigned num_insns = mon_get_number_of_insns(&monitor->cpu_monitor[cpu_nr]);

    total_insns += num_insns;
    len = strlen (mon_add_commas(buffer, sizeof(buffer), num_insns));
    if (len_num < len)
      len_num = len;
  }
  
  sprintf (buffer, "%d", (int)monitor->nr_cpus + 1);
  len_cpu = strlen (buffer);

#ifdef HAVE_GETRUSAGE
  if (total_insns && verbose > 1)
    {
      struct rusage mytime;
      if (getrusage (RUSAGE_SELF, &mytime) == 0
	  && (mytime.ru_utime.tv_sec > 0 || mytime.ru_utime.tv_usec > 0)) {

	cpu_time = (double)mytime.ru_utime.tv_sec + (((double)mytime.ru_utime.tv_usec) / 1000000.0);
	cpu_insns_second = (long)(((double)total_insns / cpu_time) + 0.5);
      }
  }
#endif

  for (cpu_nr = 0; cpu_nr < monitor->nr_cpus; cpu_nr++) {

    if (verbose > 1) {
      itable_index index;

      if (cpu_nr)
	printf_filtered ("\n");

      for (index = 0; index < nr_itable_entries; index++) {
	if (monitor->cpu_monitor[cpu_nr].issue_count[index])
	  printf_filtered("CPU #%*d executed %*s %s instruction%s.\n",
			  len_cpu, cpu_nr+1,
			  len_num, mon_add_commas(buffer,
						  sizeof(buffer),
						  monitor->cpu_monitor[cpu_nr].issue_count[index]),
			  itable[index].name,
			  (monitor->cpu_monitor[cpu_nr].issue_count[index] == 1) ? "" : "s");
      }

      if (monitor->cpu_monitor[cpu_nr].read_count)
	printf_filtered ("CPU #%*d executed %*s data reads.\n",
			 len_cpu, cpu_nr+1,
			 len_num, mon_add_commas(buffer,
						 sizeof(buffer),
						 monitor->cpu_monitor[cpu_nr].read_count));

      if (monitor->cpu_monitor[cpu_nr].write_count)
	printf_filtered ("CPU #%*d executed %*s data writes.\n",
			 len_cpu, cpu_nr+1,
			 len_num, mon_add_commas(buffer,
						 sizeof(buffer),
						 monitor->cpu_monitor[cpu_nr].write_count));
    }
    
    printf_filtered("CPU #%*d executed %*s instructions in total.\n",
		    len_cpu, cpu_nr+1,
		    len_num, mon_add_commas(buffer,
					    sizeof(buffer),
					    mon_get_number_of_insns(&monitor->cpu_monitor[cpu_nr])));
  }

  if (monitor->nr_cpus > 1)
    printf_filtered("\nAll CPUs executed %s instructions in total.\n",
		    mon_add_commas(buffer, sizeof(buffer), total_insns));

  if (cpu_insns_second)
    printf_filtered ("\nSimulator speed was %s instructions/second\n",
		     mon_add_commas(buffer, sizeof(buffer), cpu_insns_second));
}

#endif /* _MON_C_ */
