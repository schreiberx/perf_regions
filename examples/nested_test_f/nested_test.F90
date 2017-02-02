PROGRAM main
!pragma perf_region include
 
CALL timing_init()
call test1
!call test2

CALL timing_finalize()
 
END PROGRAM main

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SUBROUTINE test1

!pragma perf_region include
 
CALL timing_start('FOOa');
CALL timing_start('FOOb');
       call test2
CALL timing_stop('FOOb');
CALL timing_stop('FOOa');

end SUBROUTINE test1

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SUBROUTINE test2
!pragma perf_region include
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
              
CALL timing_start('BARa');

CALL timing_start('BARb');

                     DO i=1, array_size 
                          a(i) = a(i) + a(i)*a(i)
                     ENDDO

CALL timing_stop('BARb');

CALL timing_stop('BARa');

                     fac = 1.0;

              ENDDO
              deallocate(a)
              array_size = array_size*2
      ENDDO

end subroutine test2