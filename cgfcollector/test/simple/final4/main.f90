module mod
    implicit none

    type dummy_type
        integer :: i
    contains
        final :: finalize_dummy
    end type dummy_type

    interface dummy_type
        module procedure create_dummy_type
    end interface dummy_type
contains
    subroutine finalize_dummy(this)
        type(dummy_type), intent(inout) :: this
        write (*, *) 'Finalizing dummy_type'
    end subroutine finalize_dummy

    type(dummy_type) function create_dummy_type(i)
        integer, intent(in) :: i
        create_dummy_type%i = i
        write (*, *) 'Creating dummy_type with i = ', create_dummy_type%i
    end function create_dummy_type
end module mod

program main
    use mod

    implicit none

    type(dummy_type), allocatable :: dummy
    dummy = dummy_type(1)

    ! dummy finalizer is called here but this is not confrom with the fortran specification (7.5.6.4)
end program main

