MODULE perf_regions_fortran

# include "perf_region_defines.h"

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

END MODULE