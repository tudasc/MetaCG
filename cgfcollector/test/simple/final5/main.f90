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

    type(derived) :: derivedInModule ! not destructed because static lifetime

contains

    subroutine finalize_base(this)
        type(base), intent(inout) :: this
        print *, "Finalizing base"
    end subroutine finalize_base

    subroutine finalize_derived(this)
        type(derived), intent(inout) :: this
        print *, "Finalizing derived"
    end subroutine finalize_derived

end module mod

program main

    implicit none

contains
    subroutine func()
        use mod
        type(derived), save :: obj ! save = static lifetime

        obj = derived(1, 2)
    end subroutine func

end program main

