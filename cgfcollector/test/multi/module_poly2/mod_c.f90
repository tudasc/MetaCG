module mod_c
    use mod_b
    implicit none

    type, extends(middle) :: end
    contains
        procedure :: op => op_end
    end type end

contains
    subroutine op_end(this)
        class(end), intent(in) :: this
        print *, "end"
    end subroutine op_end
end module mod_c

