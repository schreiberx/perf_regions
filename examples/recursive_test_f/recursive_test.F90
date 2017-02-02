PROGRAM main
!PERF_REGION_ORIGINAL
!!pragma perf_region include
!PERF_REGION_CODE
USE perf_regions_fortran
!PERF_REGION_ORIGINAL
![PERF_REGION_DUMMY]
!PERF_REGION_CODE
#include "perf_region_defines.h"
 
!PERF_REGION_ORIGINAL
!CALL timing_init()
!PERF_REGION_CODE
CALL perf_regions_init()
call test1
!call test2

!PERF_REGION_ORIGINAL
!CALL timing_finalize()
!PERF_REGION_CODE
CALL perf_regions_finalize()
 
END PROGRAM main

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SUBROUTINE test1

!PERF_REGION_ORIGINAL
!!pragma perf_region include
!PERF_REGION_CODE
USE perf_regions_fortran
!PERF_REGION_ORIGINAL
![PERF_REGION_DUMMY]
!PERF_REGION_CODE
#include "perf_region_defines.h"
 
!PERF_REGION_ORIGINAL
!CALL timing_start('FOOa');
!PERF_REGION_CODE
CALL perf_region_start(0, IOR(INT(PERF_FLAG_TIMINGS), INT(PERF_FLAG_COUNTERS))) !FOOA
!PERF_REGION_ORIGINAL
!CALL timing_start('FOOb');
!PERF_REGION_CODE
CALL perf_region_start(1, IOR(INT(PERF_FLAG_TIMINGS), INT(PERF_FLAG_COUNTERS))) !FOOB
       call test2
!PERF_REGION_ORIGINAL
!CALL timing_stop('FOOb');
!PERF_REGION_CODE
CALL perf_region_stop(1) !FOOB
!PERF_REGION_ORIGINAL
!CALL timing_stop('FOOa');
!PERF_REGION_CODE
CALL perf_region_stop(0) !FOOA

end SUBROUTINE test1

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SUBROUTINE test2
!PERF_REGION_ORIGINAL
!!pragma perf_region include
!PERF_REGION_CODE
USE perf_regions_fortran
!PERF_REGION_ORIGINAL
![PERF_REGION_DUMMY]
!PERF_REGION_CODE
#include "perf_region_defines.h"
#include "perf_region_defines.h"
 
       integer :: array_size, i
       REAL*8,ALLOCATABLE  :: a(:)
       REAL*8 :: fac
       array_size=1
       WRITE(*,*) 'test',1024*1024*128


       DO WHILE(array_size < 1024*1024*128/4)
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
!CALL timing_start('BARa');
!PERF_REGION_CODE
CALL perf_region_start(2, IOR(INT(PERF_FLAG_TIMINGS), INT(PERF_FLAG_COUNTERS))) !BARA

!PERF_REGION_ORIGINAL
!CALL timing_start('BARb');
!PERF_REGION_CODE
CALL perf_region_start(3, IOR(INT(PERF_FLAG_TIMINGS), INT(PERF_FLAG_COUNTERS))) !BARB

                     DO i=1, array_size 
                          a(i) = a(i) + a(i)*a(i)
                     ENDDO

!PERF_REGION_ORIGINAL
!CALL timing_stop('BARb');
!PERF_REGION_CODE
CALL perf_region_stop(3) !BARB

!PERF_REGION_ORIGINAL
!CALL timing_stop('BARa');
!PERF_REGION_CODE
CALL perf_region_stop(2) !BARA

                     fac = 1.0;

              ENDDO
              deallocate(a)
              array_size = array_size*2
      ENDDO

end subroutine test2