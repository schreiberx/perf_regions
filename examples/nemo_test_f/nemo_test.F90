#include "../src/perf_region_defines.h"

PROGRAM nemo

     interface
        subroutine perf_regions_init ( ) bind ( C,       &
     & name ="perf_regions_init" )
                use iso_c_binding
        end subroutine perf_regions_init


        subroutine perf_regions_finalize ( ) bind ( C,       &
     & name ="perf_regions_finalize" )
                use iso_c_binding
        end subroutine perf_regions_finalize


        subroutine perf_region_start (id, measure_type) bind ( C,       &
     & name ="perf_region_start" )
                use iso_c_binding
                INTEGER, VALUE, INTENT(IN) ::id
                INTEGER, VALUE, INTENT(IN) ::measure_type
        end subroutine perf_region_start

        subroutine perf_region_stop (id) bind ( C,        &
     & name ="perf_region_stop" )
                use iso_c_binding
                INTEGER, VALUE, INTENT(IN) ::id
        end subroutine perf_region_stop

      end interface

     call perf_regions_init()

     CALL testfoo
     CALL testbar
     CALL testxyz

     CALL testfoo
     CALL testbar
     CALL testxyz

     call perf_regions_finalize()

CONTAINS

   subroutine testfoo

   REAL, ALLOCATABLE, SAVE, DIMENSION(:,:,:) :: tsn, pun, pvn, pwn
   REAL, ALLOCATABLE, SAVE, DIMENSION(:,:,:) :: mydomain, zslpx, zslpy, zwx, zwy, umask, vmask, tmask, zind
   REAL, ALLOCATABLE, SAVE, DIMENSION(:,:)   :: ztfreez, rnfmsk, upsmsk
   REAL, ALLOCATABLE, SAVE, DIMENSION(:)     :: rnfmsk_z
   REAL                                      :: zice
   REAL                                      ::   zu, z0u, zzwx
   REAL                                      ::   zv, z0v, zzwy
   REAL                                      ::   ztra, zbtr, zdt, zalpha
   INTEGER                                   :: jpi, jpj, jpk
   INTEGER                                   :: ji, jj, jk
   CHARACTER(len=10)                         :: env

   IF( nn_timing == 1 )  CALL timing_start("testfoo")

   CALL get_environment_variable("JPI", env)
   READ ( env, '(i10)' ) jpi
   CALL get_environment_variable("JPJ", env)
   READ ( env, '(i10)' ) jpj
   CALL get_environment_variable("JPK", env)
   READ ( env, '(i10)' ) jpk

   ALLOCATE( mydomain (jpi,jpj,jpk))
   ALLOCATE( zwx (jpi,jpj,jpk))
   ALLOCATE( zwy (jpi,jpj,jpk))
   ALLOCATE( zslpx (jpi,jpj,jpk))
   ALLOCATE( zslpy (jpi,jpj,jpk))
   ALLOCATE( pun (jpi,jpj,jpk))
   ALLOCATE( pvn (jpi,jpj,jpk))
   ALLOCATE( pwn (jpi,jpj,jpk))
   ALLOCATE( umask (jpi,jpj,jpk))
   ALLOCATE( vmask (jpi,jpj,jpk))
   ALLOCATE( tmask (jpi,jpj,jpk))
   ALLOCATE( zind (jpi,jpj,jpk))
   ALLOCATE( ztfreez (jpi,jpj))
   ALLOCATE( rnfmsk (jpi,jpj))
   ALLOCATE( upsmsk (jpi,jpj))
   ALLOCATE( rnfmsk_z (jpk))
   ALLOCATE( tsn(jpi,jpj,jpk))

! array initialization
   DO jk = 1, jpk
     DO jj = 1, jpj
       DO ji = 1, jpi
         tsn(ji,jj,jk)= ji*jj*jk
         mydomain(ji,jj,jk) =ji*jj*jk
         umask(ji,jj,jk) = ji*jj*jk
         vmask(ji,jj,jk)= ji*jj*jk
         tmask(ji,jj,jk)= ji*jj*jk
         pun(ji,jj,jk) =ji*jj*jk
         pvn(ji,jj,jk) =ji*jj*jk
         pwn(ji,jj,jk) =ji*jj*jk
       END DO
     END DO
   END DO

   DO jj=1, jpj
     DO ji=1, jpi
       ztfreez(ji,jj)=ji*jj
       upsmsk(ji,jj)=ji*jj
       rnfmsk(ji,jj) = ji*jj
     END DO
   END DO

   DO jk=1, jpk
     rnfmsk_z(jk)=jk
   END DO


!***********************
!* Start of the synphony
!***********************

   DO jk = 1, jpk
     DO jj = 1, jpj
       DO ji = 1, jpi
         IF( tsn(ji,jj,jk) <= ztfreez(ji,jj) + 0.1 ) THEN   ;   zice = 1.e0
         ELSE                                                      ;   zice = 0.e0
         ENDIF
         zind(ji,jj,jk) = MAX (   &
           rnfmsk(ji,jj) * rnfmsk_z(jk),      & 
           upsmsk(ji,jj)               ,      &
           zice                               &
           &                  ) * tmask(ji,jj,jk)
           zind(ji,jj,jk) = 1 - zind(ji,jj,jk)
         END DO
       END DO
     END DO

     zwx(:,:,jpk) = 0.e0   ;   zwy(:,:,jpk) = 0.e0 

     DO jk = 1, jpk-1
       DO jj = 1, jpj-1
         DO ji = 1, jpi-1
           zwx(ji,jj,jk) = umask(ji,jj,jk) * ( mydomain(ji+1,jj,jk) - mydomain(ji,jj,jk) )
           zwy(ji,jj,jk) = vmask(ji,jj,jk) * ( mydomain(ji,jj+1,jk) - mydomain(ji,jj,jk) )
         END DO
       END DO
     END DO

     zslpx(:,:,jpk) = 0.e0   ;   zslpy(:,:,jpk) = 0.e0

     DO jk = 1, jpk-1
       DO jj = 2, jpj
         DO ji = 2, jpi
           zslpx(ji,jj,jk) =                    ( zwx(ji,jj,jk) + zwx(ji-1,jj  ,jk) )   &
             &            * ( 0.25 + SIGN( 0.25, zwx(ji,jj,jk) * zwx(ji-1,jj  ,jk) ) )
           zslpy(ji,jj,jk) =                    ( zwy(ji,jj,jk) + zwy(ji  ,jj-1,jk) )   &
             &            * ( 0.25 + SIGN( 0.25, zwy(ji,jj,jk) * zwy(ji  ,jj-1,jk) ) )
         END DO
       END DO
     END DO

     DO jk = 1, jpk-1
       DO jj = 2, jpj
         DO ji = 2, jpi
           zslpx(ji,jj,jk) = SIGN( 1., zslpx(ji,jj,jk) ) * MIN(    ABS( zslpx(ji  ,jj,jk) ),   &
             &                                                 2.*ABS( zwx  (ji-1,jj,jk) ),   &
             &                                                 2.*ABS( zwx  (ji  ,jj,jk) ) )
           zslpy(ji,jj,jk) = SIGN( 1., zslpy(ji,jj,jk) ) * MIN(    ABS( zslpy(ji,jj  ,jk) ),   &
             &                                                 2.*ABS( zwy  (ji,jj-1,jk) ),   &
             &                                                 2.*ABS( zwy  (ji,jj  ,jk) ) )
         END DO
       END DO
     END DO 

     DO jk = 1, jpk-1
       zdt  = 1
       DO jj = 2, jpj-1
         DO ji = 2, jpi-1
           z0u = SIGN( 0.5, pun(ji,jj,jk) )
           zalpha = 0.5 - z0u
           zu  = z0u - 0.5 * pun(ji,jj,jk) * zdt

           zzwx = mydomain(ji+1,jj,jk) + zind(ji,jj,jk) * (zu * zslpx(ji+1,jj,jk))
           zzwy = mydomain(ji  ,jj,jk) + zind(ji,jj,jk) * (zu * zslpx(ji  ,jj,jk))

           zwx(ji,jj,jk) = pun(ji,jj,jk) * ( zalpha * zzwx + (1.-zalpha) * zzwy )
           z0v = SIGN( 0.5, pvn(ji,jj,jk) )
           zalpha = 0.5 - z0v
           zv  = z0v - 0.5 * pvn(ji,jj,jk) * zdt

           zzwx = mydomain(ji,jj+1,jk) + zind(ji,jj,jk) * (zv * zslpy(ji,jj+1,jk))
           zzwy = mydomain(ji,jj  ,jk) + zind(ji,jj,jk) * (zv * zslpy(ji,jj  ,jk))

           zwy(ji,jj,jk) = pvn(ji,jj,jk) * ( zalpha * zzwx + (1.-zalpha) * zzwy )
         END DO
       END DO
     END DO

     DO jk = 1, jpk-1
       DO jj = 2, jpj-1     
         DO ji = 2, jpi-1
           zbtr = 1.
           ztra = - zbtr * ( zwx(ji,jj,jk) - zwx(ji-1,jj  ,jk  )   &
            &               + zwy(ji,jj,jk) - zwy(ji  ,jj-1,jk  ) )
           mydomain(ji,jj,jk) = mydomain(ji,jj,jk) + ztra
         END DO
       END DO
     END DO

     zwx (:,:, 1 ) = 0.e0    ;    zwx (:,:,jpk) = 0.e0

     DO jk = 2, jpk-1
       zwx(:,:,jk) = tmask(:,:,jk) * ( mydomain(:,:,jk-1) - mydomain(:,:,jk) )
     END DO

     zslpx(:,:,1) = 0.e0

     DO jk = 2, jpk-1
       DO jj = 1, jpj
         DO ji = 1, jpi
           zslpx(ji,jj,jk) =                    ( zwx(ji,jj,jk) + zwx(ji,jj,jk+1) )   &
              &            * ( 0.25 + SIGN( 0.25, zwx(ji,jj,jk) * zwx(ji,jj,jk+1) ) )
         END DO
       END DO
     END DO

     DO jk = 2, jpk-1
       DO jj = 1, jpj
         DO ji = 1, jpi
           zslpx(ji,jj,jk) = SIGN( 1., zslpx(ji,jj,jk) ) * MIN(    ABS( zslpx(ji,jj,jk  ) ),   &
                &                                                 2.*ABS( zwx  (ji,jj,jk+1) ),   &
                &                                                 2.*ABS( zwx  (ji,jj,jk  ) )  )
         END DO
       END DO
     END DO

     zwx(:,:, 1 ) = pwn(:,:,1) * mydomain(:,:,1)

     zdt  = 1
     zbtr = 1.
     DO jk = 1, jpk-1
       DO jj = 2, jpj-1     
         DO ji = 2, jpi-1
           z0w = SIGN( 0.5, pwn(ji,jj,jk+1) )
           zalpha = 0.5 + z0w
           zw  = z0w - 0.5 * pwn(ji,jj,jk+1) * zdt * zbtr

           zzwx = mydomain(ji,jj,jk+1) + zind(ji,jj,jk) * (zw * zslpx(ji,jj,jk+1))
           zzwy = mydomain(ji,jj,jk  ) + zind(ji,jj,jk) * (zw * zslpx(ji,jj,jk  ))

           zwx(ji,jj,jk+1) = pwn(ji,jj,jk+1) * ( zalpha * zzwx + (1.-zalpha) * zzwy )
         END DO
       END DO
     END DO

     zbtr = 1.
     DO jk = 1, jpk-1
       DO jj = 2, jpj-1     
         DO ji = 2, jpi-1
           ztra = - zbtr * ( zwx(ji,jj,jk) - zwx(ji,jj,jk+1) )
           mydomain(ji,jj,jk) =  mydomain(ji,jj,jk) + ztra
         END DO
       END DO
     END DO

     DEALLOCATE(mydomain)
     DEALLOCATE(zwx)
     DEALLOCATE(zwy)
     DEALLOCATE(zslpx)
     DEALLOCATE(zslpy)
     DEALLOCATE(pun)
     DEALLOCATE(pvn)
     DEALLOCATE(pwn)
     DEALLOCATE(umask)
     DEALLOCATE(vmask)
     DEALLOCATE(tmask)
     DEALLOCATE(zind)
     DEALLOCATE(ztfreez)
     DEALLOCATE(rnfmsk)
     DEALLOCATE(upsmsk)
     DEALLOCATE(rnfmsk_z)
     DEALLOCATE(tsn)

     IF( nn_timing == 1 )  CALL timing_stop("testfoo")

     end subroutine testfoo



   subroutine testbar

   REAL, ALLOCATABLE, SAVE, DIMENSION(:,:,:) :: tsn, pun, pvn, pwn
   REAL, ALLOCATABLE, SAVE, DIMENSION(:,:,:) :: mydomain, zslpx, zslpy, zwx, zwy, umask, vmask, tmask, zind
   REAL, ALLOCATABLE, SAVE, DIMENSION(:,:)   :: ztfreez, rnfmsk, upsmsk
   REAL, ALLOCATABLE, SAVE, DIMENSION(:)     :: rnfmsk_z
   REAL                                      :: zice
   REAL                                      ::   zu, z0u, zzwx
   REAL                                      ::   zv, z0v, zzwy
   REAL                                      ::   ztra, zbtr, zdt, zalpha
   INTEGER                                   :: jpi, jpj, jpk
   INTEGER                                   :: ji, jj, jk
   CHARACTER(len=10)                         :: env

   IF( nn_timing == 1 )  CALL timing_start("testbar")

   CALL get_environment_variable("JPI", env)
   READ ( env, '(i10)' ) jpi
   CALL get_environment_variable("JPJ", env)
   READ ( env, '(i10)' ) jpj
   CALL get_environment_variable("JPK", env)
   READ ( env, '(i10)' ) jpk

   ALLOCATE( mydomain (jpi,jpj,jpk))
   ALLOCATE( zwx (jpi,jpj,jpk))
   ALLOCATE( zwy (jpi,jpj,jpk))
   ALLOCATE( zslpx (jpi,jpj,jpk))
   ALLOCATE( zslpy (jpi,jpj,jpk))
   ALLOCATE( pun (jpi,jpj,jpk))
   ALLOCATE( pvn (jpi,jpj,jpk))
   ALLOCATE( pwn (jpi,jpj,jpk))
   ALLOCATE( umask (jpi,jpj,jpk))
   ALLOCATE( vmask (jpi,jpj,jpk))
   ALLOCATE( tmask (jpi,jpj,jpk))
   ALLOCATE( zind (jpi,jpj,jpk))
   ALLOCATE( ztfreez (jpi,jpj))
   ALLOCATE( rnfmsk (jpi,jpj))
   ALLOCATE( upsmsk (jpi,jpj))
   ALLOCATE( rnfmsk_z (jpk))
   ALLOCATE( tsn(jpi,jpj,jpk))

! array initialization
   DO jk = 1, jpk
     DO jj = 1, jpj
       DO ji = 1, jpi
         tsn(ji,jj,jk)= ji*jj*jk
         mydomain(ji,jj,jk) =ji*jj*jk
         umask(ji,jj,jk) = ji*jj*jk
         vmask(ji,jj,jk)= ji*jj*jk
         tmask(ji,jj,jk)= ji*jj*jk
         pun(ji,jj,jk) =ji*jj*jk
         pvn(ji,jj,jk) =ji*jj*jk
         pwn(ji,jj,jk) =ji*jj*jk
       END DO
     END DO
   END DO

   DO jj=1, jpj
     DO ji=1, jpi
       ztfreez(ji,jj)=ji*jj
       upsmsk(ji,jj)=ji*jj
       rnfmsk(ji,jj) = ji*jj
     END DO
   END DO

   DO jk=1, jpk
     rnfmsk_z(jk)=jk
   END DO


!***********************
!* Start of the synphony
!***********************

   DO jk = 1, jpk
     DO jj = 1, jpj
       DO ji = 1, jpi
         IF( tsn(ji,jj,jk) <= ztfreez(ji,jj) + 0.1 ) THEN   ;   zice = 1.e0
         ELSE                                                      ;   zice = 0.e0
         ENDIF
         zind(ji,jj,jk) = MAX (   &
           rnfmsk(ji,jj) * rnfmsk_z(jk),      &
           upsmsk(ji,jj)               ,      &
           zice                               &
           &                  ) * tmask(ji,jj,jk)
           zind(ji,jj,jk) = 1 - zind(ji,jj,jk)
         END DO
       END DO
     END DO

     zwx(:,:,jpk) = 0.e0   ;   zwy(:,:,jpk) = 0.e0

     DO jk = 1, jpk-1
       DO jj = 1, jpj-1
         DO ji = 1, jpi-1
           zwx(ji,jj,jk) = umask(ji,jj,jk) * ( mydomain(ji+1,jj,jk) - mydomain(ji,jj,jk) )
           zwy(ji,jj,jk) = vmask(ji,jj,jk) * ( mydomain(ji,jj+1,jk) - mydomain(ji,jj,jk) )
         END DO
       END DO
     END DO

     zslpx(:,:,jpk) = 0.e0   ;   zslpy(:,:,jpk) = 0.e0

     DO jk = 1, jpk-1
       DO jj = 2, jpj
         DO ji = 2, jpi
           zslpx(ji,jj,jk) =                    ( zwx(ji,jj,jk) + zwx(ji-1,jj  ,jk) )   &
             &            * ( 0.25 + SIGN( 0.25, zwx(ji,jj,jk) * zwx(ji-1,jj  ,jk) ) )
           zslpy(ji,jj,jk) =                    ( zwy(ji,jj,jk) + zwy(ji  ,jj-1,jk) )   &
             &            * ( 0.25 + SIGN( 0.25, zwy(ji,jj,jk) * zwy(ji  ,jj-1,jk) ) )
         END DO
       END DO
     END DO

     DO jk = 1, jpk-1
       DO jj = 2, jpj
         DO ji = 2, jpi
           zslpx(ji,jj,jk) = SIGN( 1., zslpx(ji,jj,jk) ) * MIN(    ABS( zslpx(ji  ,jj,jk) ),   &
             &                                                 2.*ABS( zwx  (ji-1,jj,jk) ),   &
             &                                                 2.*ABS( zwx  (ji  ,jj,jk) ) )
           zslpy(ji,jj,jk) = SIGN( 1., zslpy(ji,jj,jk) ) * MIN(    ABS( zslpy(ji,jj  ,jk) ),   &
             &                                                 2.*ABS( zwy  (ji,jj-1,jk) ),   &
             &                                                 2.*ABS( zwy  (ji,jj  ,jk) ) )
         END DO
       END DO
     END DO

     DO jk = 1, jpk-1
       zdt  = 1
       DO jj = 2, jpj-1
         DO ji = 2, jpi-1
           z0u = SIGN( 0.5, pun(ji,jj,jk) )
           zalpha = 0.5 - z0u
           zu  = z0u - 0.5 * pun(ji,jj,jk) * zdt

           zzwx = mydomain(ji+1,jj,jk) + zind(ji,jj,jk) * (zu * zslpx(ji+1,jj,jk))
           zzwy = mydomain(ji  ,jj,jk) + zind(ji,jj,jk) * (zu * zslpx(ji  ,jj,jk))

           zwx(ji,jj,jk) = pun(ji,jj,jk) * ( zalpha * zzwx + (1.-zalpha) * zzwy )
           z0v = SIGN( 0.5, pvn(ji,jj,jk) )
           zalpha = 0.5 - z0v
           zv  = z0v - 0.5 * pvn(ji,jj,jk) * zdt

           zzwx = mydomain(ji,jj+1,jk) + zind(ji,jj,jk) * (zv * zslpy(ji,jj+1,jk))
           zzwy = mydomain(ji,jj  ,jk) + zind(ji,jj,jk) * (zv * zslpy(ji,jj  ,jk))

           zwy(ji,jj,jk) = pvn(ji,jj,jk) * ( zalpha * zzwx + (1.-zalpha) * zzwy )
         END DO
       END DO
     END DO

     DO jk = 1, jpk-1
       DO jj = 2, jpj-1
         DO ji = 2, jpi-1
           zbtr = 1.
           ztra = - zbtr * ( zwx(ji,jj,jk) - zwx(ji-1,jj  ,jk  )   &
            &               + zwy(ji,jj,jk) - zwy(ji  ,jj-1,jk  ) )
           mydomain(ji,jj,jk) = mydomain(ji,jj,jk) + ztra
         END DO
       END DO
     END DO

     zwx (:,:, 1 ) = 0.e0    ;    zwx (:,:,jpk) = 0.e0

     DO jk = 2, jpk-1
       zwx(:,:,jk) = tmask(:,:,jk) * ( mydomain(:,:,jk-1) - mydomain(:,:,jk) )
     END DO

     zslpx(:,:,1) = 0.e0

     DO jk = 2, jpk-1
       DO jj = 1, jpj
         DO ji = 1, jpi
           zslpx(ji,jj,jk) =                    ( zwx(ji,jj,jk) + zwx(ji,jj,jk+1) )   &
              &            * ( 0.25 + SIGN( 0.25, zwx(ji,jj,jk) * zwx(ji,jj,jk+1) ) )
         END DO
       END DO
     END DO

     DO jk = 2, jpk-1
       DO jj = 1, jpj
         DO ji = 1, jpi
           zslpx(ji,jj,jk) = SIGN( 1., zslpx(ji,jj,jk) ) * MIN(    ABS( zslpx(ji,jj,jk  ) ),   &
                &                                                 2.*ABS( zwx  (ji,jj,jk+1) ),   &
                &                                                 2.*ABS( zwx  (ji,jj,jk  ) )  )
         END DO
       END DO
     END DO

     zwx(:,:, 1 ) = pwn(:,:,1) * mydomain(:,:,1)

     zdt  = 1
     zbtr = 1.
     DO jk = 1, jpk-1
       DO jj = 2, jpj-1
         DO ji = 2, jpi-1
           z0w = SIGN( 0.5, pwn(ji,jj,jk+1) )
           zalpha = 0.5 + z0w
           zw  = z0w - 0.5 * pwn(ji,jj,jk+1) * zdt * zbtr

           zzwx = mydomain(ji,jj,jk+1) + zind(ji,jj,jk) * (zw * zslpx(ji,jj,jk+1))
           zzwy = mydomain(ji,jj,jk  ) + zind(ji,jj,jk) * (zw * zslpx(ji,jj,jk  ))

           zwx(ji,jj,jk+1) = pwn(ji,jj,jk+1) * ( zalpha * zzwx + (1.-zalpha) * zzwy )
         END DO
       END DO
     END DO

     zbtr = 1.
     DO jk = 1, jpk-1
       DO jj = 2, jpj-1
         DO ji = 2, jpi-1
           ztra = - zbtr * ( zwx(ji,jj,jk) - zwx(ji,jj,jk+1) )
           mydomain(ji,jj,jk) =  mydomain(ji,jj,jk) + ztra
         END DO
       END DO
     END DO

     DEALLOCATE(mydomain)
     DEALLOCATE(zwx)
     DEALLOCATE(zwy)
     DEALLOCATE(zslpx)
     DEALLOCATE(zslpy)
     DEALLOCATE(pun)
     DEALLOCATE(pvn)
     DEALLOCATE(pwn)
     DEALLOCATE(umask)
     DEALLOCATE(vmask)
     DEALLOCATE(tmask)
     DEALLOCATE(zind)
     DEALLOCATE(ztfreez)
     DEALLOCATE(rnfmsk)
     DEALLOCATE(upsmsk)
     DEALLOCATE(rnfmsk_z)
     DEALLOCATE(tsn)

     IF( nn_timing == 1 )  CALL timing_stop("testbar")

     end subroutine testbar


   subroutine testxyz

   REAL, ALLOCATABLE, SAVE, DIMENSION(:,:,:) :: tsn, pun, pvn, pwn
   REAL, ALLOCATABLE, SAVE, DIMENSION(:,:,:) :: mydomain, zslpx, zslpy, zwx, zwy, umask, vmask, tmask, zind
   REAL, ALLOCATABLE, SAVE, DIMENSION(:,:)   :: ztfreez, rnfmsk, upsmsk
   REAL, ALLOCATABLE, SAVE, DIMENSION(:)     :: rnfmsk_z
   REAL                                      :: zice
   REAL                                      ::   zu, z0u, zzwx
   REAL                                      ::   zv, z0v, zzwy
   REAL                                      ::   ztra, zbtr, zdt, zalpha
   INTEGER                                   :: jpi, jpj, jpk
   INTEGER                                   :: ji, jj, jk
   CHARACTER(len=10)                         :: env

   IF( nn_timing == 1 )  CALL timing_start("testxyz")

   CALL get_environment_variable("JPI", env)
   READ ( env, '(i10)' ) jpi
   CALL get_environment_variable("JPJ", env)
   READ ( env, '(i10)' ) jpj
   CALL get_environment_variable("JPK", env)
   READ ( env, '(i10)' ) jpk

   ALLOCATE( mydomain (jpi,jpj,jpk))
   ALLOCATE( zwx (jpi,jpj,jpk))
   ALLOCATE( zwy (jpi,jpj,jpk))
   ALLOCATE( zslpx (jpi,jpj,jpk))
   ALLOCATE( zslpy (jpi,jpj,jpk))
   ALLOCATE( pun (jpi,jpj,jpk))
   ALLOCATE( pvn (jpi,jpj,jpk))
   ALLOCATE( pwn (jpi,jpj,jpk))
   ALLOCATE( umask (jpi,jpj,jpk))
   ALLOCATE( vmask (jpi,jpj,jpk))
   ALLOCATE( tmask (jpi,jpj,jpk))
   ALLOCATE( zind (jpi,jpj,jpk))
   ALLOCATE( ztfreez (jpi,jpj))
   ALLOCATE( rnfmsk (jpi,jpj))
   ALLOCATE( upsmsk (jpi,jpj))
   ALLOCATE( rnfmsk_z (jpk))
   ALLOCATE( tsn(jpi,jpj,jpk))

! array initialization
   DO jk = 1, jpk
     DO jj = 1, jpj
       DO ji = 1, jpi
         tsn(ji,jj,jk)= ji*jj*jk
         mydomain(ji,jj,jk) =ji*jj*jk
         umask(ji,jj,jk) = ji*jj*jk
         vmask(ji,jj,jk)= ji*jj*jk
         tmask(ji,jj,jk)= ji*jj*jk
         pun(ji,jj,jk) =ji*jj*jk
         pvn(ji,jj,jk) =ji*jj*jk
         pwn(ji,jj,jk) =ji*jj*jk
       END DO
     END DO
   END DO

   DO jj=1, jpj
     DO ji=1, jpi
       ztfreez(ji,jj)=ji*jj
       upsmsk(ji,jj)=ji*jj
       rnfmsk(ji,jj) = ji*jj
     END DO
   END DO

   DO jk=1, jpk
     rnfmsk_z(jk)=jk
   END DO


!***********************
!* Start of the synphony
!***********************

   DO jk = 1, jpk
     DO jj = 1, jpj
       DO ji = 1, jpi
         IF( tsn(ji,jj,jk) <= ztfreez(ji,jj) + 0.1 ) THEN   ;   zice = 1.e0
         ELSE                                                      ;   zice = 0.e0
         ENDIF
         zind(ji,jj,jk) = MAX (   &
           rnfmsk(ji,jj) * rnfmsk_z(jk),      &
           upsmsk(ji,jj)               ,      &
           zice                               &
           &                  ) * tmask(ji,jj,jk)
           zind(ji,jj,jk) = 1 - zind(ji,jj,jk)
         END DO
       END DO
     END DO

     zwx(:,:,jpk) = 0.e0   ;   zwy(:,:,jpk) = 0.e0

     DO jk = 1, jpk-1
       DO jj = 1, jpj-1
         DO ji = 1, jpi-1
           zwx(ji,jj,jk) = umask(ji,jj,jk) * ( mydomain(ji+1,jj,jk) - mydomain(ji,jj,jk) )
           zwy(ji,jj,jk) = vmask(ji,jj,jk) * ( mydomain(ji,jj+1,jk) - mydomain(ji,jj,jk) )
         END DO
       END DO
     END DO

     zslpx(:,:,jpk) = 0.e0   ;   zslpy(:,:,jpk) = 0.e0

     DO jk = 1, jpk-1
       DO jj = 2, jpj
         DO ji = 2, jpi
           zslpx(ji,jj,jk) =                    ( zwx(ji,jj,jk) + zwx(ji-1,jj  ,jk) )   &
             &            * ( 0.25 + SIGN( 0.25, zwx(ji,jj,jk) * zwx(ji-1,jj  ,jk) ) )
           zslpy(ji,jj,jk) =                    ( zwy(ji,jj,jk) + zwy(ji  ,jj-1,jk) )   &
             &            * ( 0.25 + SIGN( 0.25, zwy(ji,jj,jk) * zwy(ji  ,jj-1,jk) ) )
         END DO
       END DO
     END DO

     DO jk = 1, jpk-1
       DO jj = 2, jpj
         DO ji = 2, jpi
           zslpx(ji,jj,jk) = SIGN( 1., zslpx(ji,jj,jk) ) * MIN(    ABS( zslpx(ji  ,jj,jk) ),   &
             &                                                 2.*ABS( zwx  (ji-1,jj,jk) ),   &
             &                                                 2.*ABS( zwx  (ji  ,jj,jk) ) )
           zslpy(ji,jj,jk) = SIGN( 1., zslpy(ji,jj,jk) ) * MIN(    ABS( zslpy(ji,jj  ,jk) ),   &
             &                                                 2.*ABS( zwy  (ji,jj-1,jk) ),   &
             &                                                 2.*ABS( zwy  (ji,jj  ,jk) ) )
         END DO
       END DO
     END DO

     DO jk = 1, jpk-1
       zdt  = 1
       DO jj = 2, jpj-1
         DO ji = 2, jpi-1
           z0u = SIGN( 0.5, pun(ji,jj,jk) )
           zalpha = 0.5 - z0u
           zu  = z0u - 0.5 * pun(ji,jj,jk) * zdt

           zzwx = mydomain(ji+1,jj,jk) + zind(ji,jj,jk) * (zu * zslpx(ji+1,jj,jk))
           zzwy = mydomain(ji  ,jj,jk) + zind(ji,jj,jk) * (zu * zslpx(ji  ,jj,jk))

           zwx(ji,jj,jk) = pun(ji,jj,jk) * ( zalpha * zzwx + (1.-zalpha) * zzwy )
           z0v = SIGN( 0.5, pvn(ji,jj,jk) )
           zalpha = 0.5 - z0v
           zv  = z0v - 0.5 * pvn(ji,jj,jk) * zdt

           zzwx = mydomain(ji,jj+1,jk) + zind(ji,jj,jk) * (zv * zslpy(ji,jj+1,jk))
           zzwy = mydomain(ji,jj  ,jk) + zind(ji,jj,jk) * (zv * zslpy(ji,jj  ,jk))

           zwy(ji,jj,jk) = pvn(ji,jj,jk) * ( zalpha * zzwx + (1.-zalpha) * zzwy )
         END DO
       END DO
     END DO

     DO jk = 1, jpk-1
       DO jj = 2, jpj-1
         DO ji = 2, jpi-1
           zbtr = 1.
           ztra = - zbtr * ( zwx(ji,jj,jk) - zwx(ji-1,jj  ,jk  )   &
            &               + zwy(ji,jj,jk) - zwy(ji  ,jj-1,jk  ) )
           mydomain(ji,jj,jk) = mydomain(ji,jj,jk) + ztra
         END DO
       END DO
     END DO

     zwx (:,:, 1 ) = 0.e0    ;    zwx (:,:,jpk) = 0.e0

     DO jk = 2, jpk-1
       zwx(:,:,jk) = tmask(:,:,jk) * ( mydomain(:,:,jk-1) - mydomain(:,:,jk) )
     END DO

     zslpx(:,:,1) = 0.e0

     DO jk = 2, jpk-1
       DO jj = 1, jpj
         DO ji = 1, jpi
           zslpx(ji,jj,jk) =                    ( zwx(ji,jj,jk) + zwx(ji,jj,jk+1) )   &
              &            * ( 0.25 + SIGN( 0.25, zwx(ji,jj,jk) * zwx(ji,jj,jk+1) ) )
         END DO
       END DO
     END DO

     DO jk = 2, jpk-1
       DO jj = 1, jpj
         DO ji = 1, jpi
           zslpx(ji,jj,jk) = SIGN( 1., zslpx(ji,jj,jk) ) * MIN(    ABS( zslpx(ji,jj,jk  ) ),   &
                &                                                 2.*ABS( zwx  (ji,jj,jk+1) ),   &
                &                                                 2.*ABS( zwx  (ji,jj,jk  ) )  )
         END DO
       END DO
     END DO

     zwx(:,:, 1 ) = pwn(:,:,1) * mydomain(:,:,1)

     zdt  = 1
     zbtr = 1.
     DO jk = 1, jpk-1
       DO jj = 2, jpj-1
         DO ji = 2, jpi-1
           z0w = SIGN( 0.5, pwn(ji,jj,jk+1) )
           zalpha = 0.5 + z0w
           zw  = z0w - 0.5 * pwn(ji,jj,jk+1) * zdt * zbtr

           zzwx = mydomain(ji,jj,jk+1) + zind(ji,jj,jk) * (zw * zslpx(ji,jj,jk+1))
           zzwy = mydomain(ji,jj,jk  ) + zind(ji,jj,jk) * (zw * zslpx(ji,jj,jk  ))

           zwx(ji,jj,jk+1) = pwn(ji,jj,jk+1) * ( zalpha * zzwx + (1.-zalpha) * zzwy )
         END DO
       END DO
     END DO

     zbtr = 1.
     DO jk = 1, jpk-1
       DO jj = 2, jpj-1
         DO ji = 2, jpi-1
           ztra = - zbtr * ( zwx(ji,jj,jk) - zwx(ji,jj,jk+1) )
           mydomain(ji,jj,jk) =  mydomain(ji,jj,jk) + ztra
         END DO
       END DO
     END DO

     DEALLOCATE(mydomain)
     DEALLOCATE(zwx)
     DEALLOCATE(zwy)
     DEALLOCATE(zslpx)
     DEALLOCATE(zslpy)
     DEALLOCATE(pun)
     DEALLOCATE(pvn)
     DEALLOCATE(pwn)
     DEALLOCATE(umask)
     DEALLOCATE(vmask)
     DEALLOCATE(tmask)
     DEALLOCATE(zind)
     DEALLOCATE(ztfreez)
     DEALLOCATE(rnfmsk)
     DEALLOCATE(upsmsk)
     DEALLOCATE(rnfmsk_z)
     DEALLOCATE(tsn)

     IF( nn_timing == 1 )  CALL timing_stop("testxyz")

     end subroutine testxyz

END PROGRAM nemo