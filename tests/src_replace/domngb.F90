MODULE ALsdfkljasljkdfdomngb
   USE wrk_nemo       ! Memory allocation
   USE timing         ! Timing

CONTAINS

   SUBROUTINE domasdlfiajsfd(kkk )
      !!----------------------------------------------------------------------
      !
      IF( nn_timing == 1 )  CALL timing_start('dom_ngb')
      !
      CALL wrk_alloc( jpi,jpj,   zglam, zgphi, zmask, zdist )
      IF( nn_timing == 1 )  CALL timing_stop('dom_ngb')
      !
   END SUBROUTINE dom_ngb

   !!======================================================================
END MODULE domngb
