module mod

    implicit none

    type, abstract :: sortable
    contains
        procedure(compare), deferred :: less_then
        generic :: operator(<) => less_then
    end type sortable

    interface
        logical function compare(this, other)
            import :: sortable
            implicit none
            class(sortable), intent(in) :: this, other
        end function compare
    end interface

    type, extends(sortable) :: integer_sortable
        integer :: value
    contains
        procedure :: less_then => less_then_integer
    end type integer_sortable

contains

    logical function less_then_integer(this, other)
        class(integer_sortable), intent(in) :: this
        class(sortable), intent(in) :: other

        select type (other)
        type is (integer_sortable)
            less_then_integer = this%value < other%value
            print *, "Comparing integer_sortable: ", this%value, " < ", other%value
        class default
            error stop "Type mismatch in comparison"
        end select
    end function less_then_integer
end module mod
