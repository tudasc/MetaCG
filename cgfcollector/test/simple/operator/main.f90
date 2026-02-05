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
        logical function compare(this, other)
            import :: sortable
            implicit none
            class(sortable), intent(in) :: this, other
        end function compare
        logical function not(this)
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

    type :: integer_wrapper
        integer :: value
    end type integer_wrapper

    interface operator(.NEGX.)
        module procedure negx
    end interface

    interface operator(+)
        procedure add_stuff, add_stuff2
        module procedure unary_plus
    end interface

contains
    logical function less_than_integer(this, other)
        class(integer_sortable), intent(in) :: this
        class(sortable), intent(in) :: other

        select type (other)
        type is (integer_sortable)
            less_than_integer = this%value < other%value
            print *, "Comparing integer_sortable: ", this%value, " < ", other%value
        class default
            error stop "Type mismatch in comparison"
        end select
    end function less_than_integer

    logical function not_impl(this)
        class(sortable), intent(in) :: this
        not_impl = .NOT. this%less_then(this)
        print *, "Hello from not_impl"
    end function not_impl

    logical function not_impl_integer(this)
        class(integer_sortable), intent(in) :: this
        not_impl_integer = .NOT. this < this
        print *, "Hello from not_impl_integer"
    end function not_impl_integer

    function negx(x) result(res)
        real, intent(in) :: x
        real :: res
        res = -x
        print *, "Hello from negx"
    end function negx

    function add_stuff(x, y) result(res)
        type(integer_sortable), intent(in) :: x, y
        type(integer_sortable) :: res
        res%value = x%value + y%value
        print *, "Hello from add_stuff"
    end function add_stuff

    function add_stuff2(x, y) result(res)
        type(integer_wrapper), intent(in) :: x, y
        type(integer_wrapper) :: res
        res%value = x%value + y%value
        print *, "Hello from add_stuff2"
    end function add_stuff2

    function unary_plus(x) result(res)
        class(integer_sortable), intent(in) :: x
        type(integer_sortable) :: res

        res%value = x%value
        print *, "Hello from unary_plus"
    end function unary_plus
end module mod

program main
    use mod

    implicit none

    call test_compare()
    call test_compare2()
    call test_not()
    call test_negx()
    call test_add()
    call test_add2()
    call test_unary_plus()
    call test_expr()

contains
    subroutine test_compare()
        class(sortable), allocatable :: a, b
        type(integer_sortable) :: c, d
        type(integer_sortable) :: res

        c%value = 5
        d%value = 10
        allocate (a, source=c)
        allocate (b, source=d)

        if (a < b) then
            print *, "a is less than b"
        else
            print *, "a is nat less than b"
        end if
    end subroutine test_compare

    subroutine test_compare2()
        class(sortable), allocatable :: a, b
        type(integer_sortable) :: c, d
        type(integer_sortable) :: res

        c%value = 5
        d%value = 10
        allocate (a, source=c)
        allocate (b, source=d)

        if (c < d) then
            print *, "c is less than d"
        else
            print *, "c is not less than d"
        end if
    end subroutine test_compare2

    subroutine test_not()
        type(integer_sortable) :: c

        if (.NOT. c) then
            print *, "c is not true"
        else
            print *, "c is true"
        end if
    end subroutine test_not

    subroutine test_negx()
        real :: e = 5.0, f
        f = .NEGX.e
        print *, "Negated value: ", f
    end subroutine test_negx

    subroutine test_add()
        type(integer_sortable) :: c, d, res
        c%value = 5
        d%value = 10
        res = c + d
        print *, "Result of addition: ", res%value
    end subroutine test_add

    subroutine test_add2()
        type(integer_wrapper) :: c, d, res
        c%value = 5
        d%value = 10
        res = c + d + c
        print *, "Result of addition: ", res%value
    end subroutine test_add2

    subroutine test_unary_plus()
        type(integer_sortable) :: c, res
        c%value = 5
        res = +c
        print *, "Result of unary plus: ", res%value
    end subroutine test_unary_plus

    subroutine test_expr()
        logical :: ad

        ad = (.NOT. 324 < 2) .EQV. .true.
    end subroutine test_expr

end program main
