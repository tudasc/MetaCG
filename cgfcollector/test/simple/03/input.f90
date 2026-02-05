module mod
    implicit none

    type :: polynomial
        private
        real, allocatable :: a(:)
    contains
        procedure :: print_polynomial
        final :: finalize_polynomial
    end type polynomial

    interface polynomial
        module procedure create_polynomial
    end interface

contains

    type(polynomial) function create_polynomial(a)
        real, intent(in) :: a(0:)
        integer :: degree(1)

        degree = findloc(a /= 0.0, value=.true., back=.true.) - 1
        allocate (create_polynomial%a(0:degree(1)))
        create_polynomial%a(0:) = a(0:degree(1))
    end function create_polynomial

    subroutine print_polynomial(this)
        class(polynomial), intent(in) :: this
        write (*, *) 'Polynomial:', this%a
    end subroutine print_polynomial

    subroutine finalize_polynomial(this)
        type(polynomial), intent(inout) :: this
        if (allocated(this%a)) then
            deallocate (this%a)
        end if
        write (*, *) 'Finalizing polynomial'
    end subroutine finalize_polynomial

end module mod

module mod_use
    use mod

    implicit none

contains

    subroutine func_calls_final()
        type(polynomial), allocatable :: q

        q = polynomial([2., 3., 1., 0., 0.])
        call q%print_polynomial()
    end subroutine func_calls_final

    subroutine func_does_not_call_final()
        type(polynomial), allocatable :: q
    end subroutine func_does_not_call_final

end module mod_use

program main
    use mod
    implicit none

    work: block
        type(polynomial), allocatable :: q

        q = polynomial([2., 3., 1., 0., 0.])
        call q%print_polynomial()
    end block work

    ! sould not call final because main. see 7.5.6.4 (https://j3-fortran.org/doc/year/23/23-007r1.pdf) and (https://j3-fortran.org/doc/year/10/10-158r1.txt)

end program main
