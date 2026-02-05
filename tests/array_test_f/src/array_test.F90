PROGRAM main

!$perf_regions include

   integer :: array_size, i
   REAL*8, ALLOCATABLE  :: a(:)
   REAL*8 :: fac
   array_size = 1
   WRITE (*, *) 'test', 1024*1024*128

   !$perf_regions init

   DO WHILE (array_size < 1024*1024*128/8)
      allocate (a(array_size))
      iters = (1024*128)/array_size
      IF (iters <= 0) iters = 2
      iters = MIN(128, iters)

      WRITE (*, *) "**************************************"
      WRITE (*, *) "Mem size: ", array_size*0.001, " KByte"
      WRITE (*, *) "Iterations: ", iters

      !initialize everything
      DO i = 1, array_size
         a(i) = i; 
      END DO

      DO i = 1, array_size
         a(i) = a(i) + a(i)*a(i)
      END DO

      DO k = 1, iters

         !$perf_regions start foo
!                     CALL timing_start('FOO');

         DO i = 1, array_size
            a(i) = a(i) + a(i)*a(i)
         END DO

!                     CALL timing_stop('FOO');
         !$perf_regions stop foo

         fac = 1.0; 
      END DO
      deallocate (a)
      array_size = array_size*2
   END DO
!             timing_finalize();

   !$perf_regions finalize

END PROGRAM main

