module mod

    implicit none

    type, abstract :: base
        integer :: value
    contains
        procedure(s), deferred :: show
    end type base

    abstract interface
        subroutine s(this)
            import :: base
            implicit none
            class(base), intent(in) :: this
        end subroutine s
    end interface

    type, extends(base) :: derived1
    contains
        procedure :: show => show_derived1
    end type derived1

    type, extends(base) :: derived2
    contains
        procedure :: show => show_derived2
    end type derived2

    interface operator(+)
        module procedure add_base
    end interface

    interface operator(-)
        module procedure sub_derived1
    end interface

contains

    function add_base(a, b) result(c)
        class(base), intent(in) :: a, b
        class(base), allocatable :: c

        select type (a)
        type is (derived1)
            allocate (derived1 :: c)
            c%value = a%value + b%value
        type is (derived2)
            allocate (derived2 :: c)
            c%value = a%value + b%value
        class default
            allocate (derived1 :: c)
            c%value = a%value + b%value
        end select
    end function add_base

    function sub_derived1(a, b) result(c)
        class(base), intent(in) :: a, b
        class(base), allocatable :: c

        allocate (derived1 :: c)
        c%value = a%value - b%value
    end function sub_derived1

    subroutine show_derived1(this)
        class(derived1), intent(in) :: this
        print *, "Derived1 with value: ", this%value
    end subroutine show_derived1

    subroutine show_derived2(this)
        class(derived2), intent(in) :: this
        print *, "Derived2 with value: ", this%value
    end subroutine show_derived2

end module mod

program main
    use mod

    implicit none

    class(base), allocatable :: obj1, obj2, result

    class(derived1), allocatable :: obj_d1
    class(base), allocatable :: obj_d2

    allocate (derived1 :: obj1)
    obj1%value = 5
    allocate (derived2 :: obj2)
    obj2%value = 7
    allocate (derived1 :: obj_d1)
    obj_d1%value = 10
    allocate (derived1 :: obj_d2)
    obj_d2%value = 10

    result = (obj1 + obj2) - obj1
    call result%show()

    result = obj_d1 + obj_d2
    call result%show()

end program main

