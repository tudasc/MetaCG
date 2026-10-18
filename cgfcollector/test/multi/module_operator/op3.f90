module mod2
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

end module mod2
