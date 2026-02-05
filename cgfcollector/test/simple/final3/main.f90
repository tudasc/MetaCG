module mod

    implicit none

    type :: base
        integer :: a
    contains
        final :: finalize_base
    end type base

    type, extends(base) :: derived
        integer :: b
    contains
        final :: finalize_derived
    end type derived

contains

    subroutine finalize_base(this)
        type(base), intent(inout) :: this
        print *, "Finalizing base"
    end subroutine finalize_base

    subroutine finalize_derived(this)
        type(derived), intent(inout) :: this
        print *, "Finalizing derived"
    end subroutine finalize_derived

    subroutine func_arg(this)
        type(derived), allocatable :: this
        print *, "In func_arg"
        allocate (this)
    end subroutine func_arg

    subroutine func_arg_out(this)
        type(derived), allocatable, intent(out) :: this
        print *, "In func_arg_out"
        allocate (this)
    end subroutine func_arg_out

    subroutine func_arg_inout(this)
        type(derived), allocatable, intent(inout) :: this
        print *, "In func_arg_inout"
        allocate (this)
    end subroutine func_arg_inout

    subroutine func_arg2(this)
        type(derived), allocatable :: this
        print *, "In func_arg2"
    end subroutine func_arg2

    subroutine func_arg_out2(this)
        type(derived), allocatable, intent(out) :: this
        print *, "In func_arg_out2"
    end subroutine func_arg_out2

    subroutine func_arg_inout2(this)
        type(derived), allocatable, intent(inout) :: this
        print *, "In func_arg_inout2"
    end subroutine func_arg_inout2

end module mod

program main
    use mod

    implicit none

    print *, "Calling func"
    call func()
    print *, "Calling func_no_finalize"
    call func_no_finalize()

contains

    subroutine func()
        type(derived), allocatable :: obj
        type(derived), allocatable :: obj2
        type(derived), allocatable :: obj3

        print *, "Before func_arg"
        call func_arg(obj)
        print *, "After func_arg"
        print *, "Before func_arg_out"
        call func_arg_out(obj2)
        print *, "After func_arg_out"
        print *, "Before func_arg_inout"
        call func_arg_inout(obj3)
        print *, "After func_arg_inout"

    end subroutine func

    subroutine func_no_finalize()
        type(derived), allocatable :: obj
        type(derived), allocatable :: obj2
        type(derived), allocatable :: obj3

        print *, "Before func_arg2"
        call func_arg2(obj)
        print *, "After func_arg2"
        print *, "Before func_arg_out2"
        call func_arg_out2(obj2)
        print *, "After func_arg_out2"
        print *, "Before func_arg_inout2"
        call func_arg_inout2(obj3)
        print *, "After func_arg_inout2"

    end subroutine func_no_finalize

    ! TODO:
    ! subroutine func_more()
    !     type(derived), allocatable :: obj

    !     call func_more_internal(obj)

    ! contains
    !     subroutine func_more_internal(obj_internal)
    !         type(derived), intent(out), allocatable :: obj_internal

    !         call func_arg_out(obj_internal)

    !     end subroutine func_more_internal
    ! end subroutine func_more

end program main

