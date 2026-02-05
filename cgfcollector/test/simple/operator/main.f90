module mod

    implicit none

    type, abstract :: sortable
    contains
        procedure(compare), deferred :: less_then
        procedure(not), deferred :: not_impl
        generic :: operator(<) => less_then
        generic :: operator(.NOT.) => not_impl
    end type sortable

    interface
        pure logical function compare(this, other)
            import :: sortable
            implicit none
            class(sortable), intent(in) :: this, other
        end function compare
        pure logical function not(this)
            import :: sortable
            implicit none
            class(sortable), intent(in) :: this
        end function not
    end interface

    type, extends(sortable) :: integer_sortable
        integer :: value
    contains
        procedure :: less_then => less_than_integer
        procedure :: not_impl => not_impl_integer
    end type integer_sortable

    interface operator(.NEGX.)
        module procedure negx
    end interface

    interface operator(+)
        module procedure add_stuff
    end interface

contains
    pure logical function less_than_integer(this, other)
        class(integer_sortable), intent(in) :: this
        class(sortable), intent(in) :: other

        select type (other)
        type is (integer_sortable)
            less_than_integer = this%value < other%value
        class default
            error stop "Type mismatch in comparison"
        end select

        ! unsafe
        ! type(integer_sortable), allocatable :: other_int
        ! other_int = transfer(other, this)
        ! less_than_integer = this%value < other_int%value

    end function less_than_integer

    pure logical function not_impl(this)
        class(sortable), intent(in) :: this
        not_impl = .NOT. this%less_then(this)
    end function not_impl

    pure logical function not_impl_integer(this)
        class(integer_sortable), intent(in) :: this
        not_impl_integer = .NOT. this < this
    end function not_impl_integer

    function negx(x) result(res)
        real, intent(in) :: x
        real :: res
        res = -x
    end function negx

    function add_stuff(x, y) result(res)
        type(integer_sortable), intent(in) :: x, y
        type(integer_sortable) :: res
        res%value = x%value + y%value
    end function add_stuff
end module mod

program main
    use mod

    implicit none

    class(sortable), allocatable :: a, b

    type(integer_sortable) :: c, d
    type(integer_sortable) :: res

    real :: e = 5.0, f

    c%value = 5
    d%value = 10

    allocate (a, source=c)
    allocate (b, source=d)

    if (a < b) then
        print *, "a is less than b"
    else
        print *, "a is not less than b"
    end if

    f = .NEGX.e
    print *, f

    res = c + d
    print *, "Result of addition: ", res%value

end program main

