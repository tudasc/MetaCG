module module0_5
    use module2, only: func2 => func

    implicit none

contains
    subroutine func()
        call func2()
        print *, "This is module2"
    end subroutine func

end module module0_5

