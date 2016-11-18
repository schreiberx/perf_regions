PROGRAM main

!PERF_REGION_ORIGINAL
!!pragma perf_region include
!PERF_REGION_CODE
USE perf_regions_fortran
!PERF_REGION_ORIGINAL
! 
!PERF_REGION_CODE
#include "perf_region_defines.h"
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

       integer :: array_size, i
       REAL*8,ALLOCATABLE  :: a(:)
       REAL*8 :: fac
       array_size=1
       WRITE(*,*) 'test',1024*1024*128

!PERF_REGION_ORIGINAL
!!             CALL timing_init();
!PERF_REGION_CODE
CALL perf_regions_init()

       DO WHILE(array_size < 1024*1024*128/8)
              allocate(a(array_size))
              iters = (1024*128)/array_size
              IF(iters <= 0) iters=2
              iters=MIN(128,iters)
                 

              WRITE(*,*) "**************************************"
              WRITE(*,*) "Mem size: ", array_size*0.001 ," KByte"
              WRITE(*,*) "Iterations: ", iters

              !initialize everything
              DO i=1,array_size
                     a(i) = i;
              ENDDO

              DO i=1, array_size 
                   a(i) = a(i) + a(i)*a(i)
              ENDDO

              DO k=1,iters
              
!PERF_REGION_ORIGINAL
!!                     CALL timing_start('FOO');
!PERF_REGION_CODE
CALL perf_region_start(0, IOR(INT(PERF_FLAG_TIMINGS), INT(PERF_FLAG_COUNTERS))) !FOO

                     DO i=1, array_size 
                          a(i) = a(i) + a(i)*a(i)
                     ENDDO

!PERF_REGION_ORIGINAL
!!                     CALL timing_stop('FOO');
!PERF_REGION_CODE
CALL perf_region_stop(0) !FOO

                     fac = 1.0;

!//                   perf_region_set_normalize(PERF_REGIONS_FOO, fac);
              ENDDO
              deallocate(a)
              array_size = array_size*2
      ENDDO
!PERF_REGION_ORIGINAL
!!             timing_finalize();
!PERF_REGION_CODE
CALL perf_regions_finalize()

END PROGRAM main