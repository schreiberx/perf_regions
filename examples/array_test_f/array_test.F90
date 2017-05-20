PROGRAM main

!pragma perf_regions include

       integer :: array_size, i
       REAL*8,ALLOCATABLE  :: a(:)
       REAL*8 :: fac
       array_size=1
       WRITE(*,*) 'test',1024*1024*128

!             CALL timing_init();

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
              
!                     CALL timing_start('FOO');

                     DO i=1, array_size 
                          a(i) = a(i) + a(i)*a(i)
                     ENDDO

!                     CALL timing_stop('FOO');

                     fac = 1.0;

!//                   perf_region_set_normalize(PERF_REGIONS_FOO, fac);
              ENDDO
              deallocate(a)
              array_size = array_size*2
      ENDDO
!             timing_finalize();

END PROGRAM main