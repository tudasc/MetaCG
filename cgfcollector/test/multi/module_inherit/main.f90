module mod2
    use mod
    implicit none

    type, extends(base) :: derived
    contains
        procedure :: set_var => set_var_derived
    end type derived

    type, extends(derived) :: more_derived
    contains
        procedure :: set_var => set_var_more_derived
    end type more_derived

contains

    subroutine set_var_derived(this, a)
        class(derived), intent(inout) :: this
        real, intent(in) :: a
        print *, 'Setting var from derived'
        this%var = a
    end subroutine set_var_derived

    subroutine set_var_more_derived(this, a)
        class(more_derived), intent(inout) :: this
        real, intent(in) :: a
        print *, 'Setting var from more_derived'
        this%var = a
    end subroutine set_var_more_derived

    subroutine set_from_item(item, a)
        class(derived), intent(inout) :: item
        real, intent(in) :: a

        print *, 'Setting var from item'
        call item%set_var(a)
    end subroutine set_from_item

end module mod2

program main
    use mod2

    implicit none

    call case1()
    call case2()

contains
    subroutine case1()
        type(derived) :: b

        call b%set_var(3.14)

        call set_from_item(b, 2.71)
    end subroutine case1

    subroutine case2()
        class(derived), allocatable :: b

        allocate (more_derived :: b)

        call b%set_var(3.14)
    end subroutine case2

end program main

