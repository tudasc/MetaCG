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
        ! this does not call a finalize
        type(derived) :: this
        print *, "In func_arg"
    end subroutine func_arg

    subroutine func_arg2(this)
        ! this calls a finalize
        type(derived) :: this
        print *, "In func_arg2"
        this = derived(1, 2)
    end subroutine func_arg2

    subroutine func_arg_out(this)
        ! this calls a finalize (7.5.6.3 line 21 and onwards)
        type(derived), intent(out) :: this
        print *, "In func_arg_out"
    end subroutine func_arg_out

    subroutine func_arg_inout(this)
        ! does not call a finalize
        type(derived), intent(inout) :: this
        print *, "In func_arg_inout"
    end subroutine func_arg_inout

    subroutine func_arg_inout2(this)
        ! this calls a finalize
        type(derived), intent(inout) :: this
        print *, "In func_arg_inout2"
        this = derived(1, 2)
    end subroutine func_arg_inout2

end module mod

program main
    use mod

    implicit none

    call func()

contains

    subroutine func()
        type(derived) :: obj

        print *, "Before func_arg"
        call func_arg(obj)
        print *, "After func_arg"
        print *, "Before func_arg2"
        call func_arg2(obj)
        print *, "After func_arg2"
        print *, "Before func_arg_out"
        call func_arg_out(obj)
        print *, "After func_arg_out"
        print *, "Before func_arg_inout"
        call func_arg_inout(obj)
        print *, "After func_arg_inout"
        print *, "Before func_arg_inout2"
        call func_arg_inout2(obj)
        print *, "After func_arg_inout2"

    end subroutine func

end program main

