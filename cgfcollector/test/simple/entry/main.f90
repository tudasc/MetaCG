module mod

    implicit none

contains
    subroutine func(a, b, c)
        integer :: a, b
        character(4) :: c
        print *, "entry func"
        return

        entry entry1(a, b, c)
        print *, "entry entry1"
        return

        entry entry2
        print *, "entry entry2"
        return
    end
end module mod

program main
    use mod
    implicit none

    call func(1, 2, "1234")
    call entry1(1, 2, "1234")
    call entry2()

end program main

