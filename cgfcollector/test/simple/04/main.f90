module mod

    implicit none

    type :: body
        private
        real :: mass
        real :: pos(3), vel(3)
    contains
        procedure :: set_mass => set_mass_body
    end type body

    type, extends(body) :: charged_body
        real :: charge
    contains
        procedure :: set_mass => set_mass_charged_body
    end type charged_body

    class(body), allocatable :: polymorphic_body

contains
    subroutine set_mass_body(this, a)
        class(body), intent(inout) :: this
        real, intent(in) :: a

        write (*, *) 'Setting mass in body'

        this%mass = a
    end subroutine set_mass_body

    subroutine set_mass_charged_body(this, a)
        class(charged_body), intent(inout) :: this
        real, intent(in) :: a

        write (*, *) 'Setting mass in charged body'

        this%mass = a
    end subroutine set_mass_charged_body

end module mod

program main
    use mod

    implicit none

    allocate (charged_body :: polymorphic_body)
    call polymorphic_body%set_mass(5.0)

end program main

