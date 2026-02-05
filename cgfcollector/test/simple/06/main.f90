module mod

    implicit none

    type, abstract :: sortable
    contains
        procedure(compare), deferred :: less_then
        generic :: operator(<) => less_then
    end type sortable

    interface
        pure logical function compare(this, other)
            import :: sortable
            class(sortable), intent(in) :: this, other
        end function compare
    end interface

    type, extends(sortable) :: integer_sortable
        integer :: value
    contains
        procedure :: less_then => less_than_integer
    end type integer_sortable

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
end module mod

program main
    use mod

    implicit none

    class(sortable), allocatable :: a, b

    type(integer_sortable) :: c, d

    c%value = 5
    d%value = 10

    allocate (a, source=c)
    allocate (b, source=d)

    if (a < b) then
        print *, "a is less than b"
    else
        print *, "a is not less than b"
    end if

end program main

