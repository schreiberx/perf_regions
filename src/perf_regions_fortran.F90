MODULE perf_regions_fortran

# include "perf_regions_defines.h"

     interface
        subroutine perf_regions_init() bind (C, name ="perf_regions_init")
                use iso_c_binding
        end subroutine perf_regions_init

        subroutine perf_regions_init_mpi_fortran(communicator) bind(C, name ="perf_regions_init_mpi_fortran")
                use iso_c_binding, only: C_INT
                INTEGER(kind=C_INT), VALUE, INTENT(IN) :: communicator
        end subroutine perf_regions_init_mpi_fortran

        subroutine perf_regions_finalize() bind(C, name ="perf_regions_finalize")
                use iso_c_binding
        end subroutine perf_regions_finalize

        subroutine perf_region_start_fortran(region_id, region_name, len) bind(C, name ="perf_region_start_fortran")
                use, intrinsic :: iso_c_binding, only: C_CHAR, C_INT, C_SIZE_T
                INTEGER(kind=C_INT), VALUE, INTENT(IN) :: region_id
                CHARACTER(kind=C_CHAR), dimension(*), INTENT(IN) ::region_name
                INTEGER(kind=C_SIZE_T), VALUE, INTENT(IN) :: len
        end subroutine perf_region_start_fortran

        subroutine perf_region_stop(region_id) bind ( C, name ="perf_region_stop")
                use iso_c_binding
                INTEGER(kind=C_INT), VALUE :: region_id
        end subroutine perf_region_stop

     end interface

        contains

        ! Use a dummy subroutine because we need to forward the length of the string
        subroutine perf_region_start(region_id, region_name)
                use, intrinsic :: iso_c_binding, only: C_SIZE_T
                INTEGER, VALUE, INTENT(IN) :: region_id
                CHARACTER(len=*), INTENT(IN) ::region_name

                INTEGER(kind=C_SIZE_T) :: len

                len = INT(len_trim(region_name), kind=C_SIZE_T)
                call perf_region_start_fortran(region_id, region_name, len)
        end subroutine perf_region_start


END MODULE
