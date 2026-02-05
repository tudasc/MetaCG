program main
    use mod
    implicit none

    class(base), allocatable :: b
    allocate (derived :: b)

    call b%set_var(3.14)

end program main

