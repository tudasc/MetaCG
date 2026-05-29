module mod

    implicit none

    type :: base
        real ::var
    contains
        procedure :: set_var => set_var_base
    end type base

contains

    subroutine set_var_base(this, a)
        class(base), intent(inout) :: this
        real, intent(in) :: a
        print *, 'Setting var from base'
        this%var = a
    end subroutine set_var_base

end module mod

