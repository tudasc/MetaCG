module mod

    implicit none

    type my_type
        integer :: value
    end type my_type

    ! making such, this doesn't crash the generator
    type(my_type), pointer :: ptr(:) => null()

end module mod

program main

    implicit none

end program main

