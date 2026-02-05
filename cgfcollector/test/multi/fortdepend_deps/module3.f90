module module3
    use module2, only: func2 => func

    implicit none

contains
    subroutine func()
        call func2()
        print *, "This is module3"
    end subroutine func

end module module3

