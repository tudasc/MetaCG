module m

    implicit none

    type :: string
        character(len=16) :: chars
    end type string

    interface len
        module procedure len_s
        module procedure len_c
    end interface

contains

    function len_s(expr) result(length)
        implicit none
        type(string), intent(in) :: expr
        integer                  :: length

        length = len_trim(expr%chars)
    end function len_s

    function len_c(expr) result(length)
        implicit none
        character(*), intent(in) :: expr
        integer                  :: length

        length = len_trim(expr)
    end function len_c

    subroutine assign_s_to_c(var, expr)
        implicit none
        character(*), intent(out) :: var
        type(string), intent(in)  :: expr
        integer                   :: i

        i = len(var)
    end subroutine assign_s_to_c

    subroutine assign_c_to_s(var, expr)
        implicit none
        type(string), intent(out)  :: var
        character(*), intent(in)   :: expr
        integer                    :: i

        i = len(var)
    end subroutine assign_c_to_s

end module m

program main

    use m
    implicit none

end program main
