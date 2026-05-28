module mod

    implicit none

    type :: base
        private
        real ::var
    contains
        procedure :: set_var => set_var_base
    end type base

    type, extends(base) :: derived
    contains
        procedure :: set_var => set_var_derived
    end type derived

contains
    subroutine set_var_base(this, a)
        class(base), intent(inout) :: this
        real, intent(in) :: a

        print *, 'Setting var from base'

        this%var = a
    end subroutine set_var_base

    subroutine set_var_derived(this, a)
        class(derived), intent(inout) :: this
        real, intent(in) :: a

        print *, 'Setting var from derived'

        this%var = a
    end subroutine set_var_derived
end module mod

program main
    use mod, only: base_rename => base, derived_rename => derived

    implicit none

    type(base_rename), allocatable :: b

    allocate (base_rename :: b)
    call b%set_var(3.14)

end program main

