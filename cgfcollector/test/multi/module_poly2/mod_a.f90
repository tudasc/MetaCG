module mod_a
    implicit none

    type, abstract :: base
    contains
        procedure(op_iface), deferred :: op
    end type base

    interface
        subroutine op_iface(this)
            import :: base
            class(base), intent(in) :: this
        end subroutine op_iface
    end interface
end module mod_a
